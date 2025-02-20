#include "ring_buffer.h"
#include "esp_log.h"

// Stream buffer handle
StreamBufferHandle_t stream_buffer = NULL;

// Original ring buffer implementation
ring_buffer_t audio_buffer;

// Stream buffer functions implementation
void stream_buffer_create(void)
{
    stream_buffer = xStreamBufferCreate(STREAM_BUFFER_SIZE, 1);
    if (stream_buffer == NULL)
    {
        ESP_LOGE("STREAM_BUFFER", "Failed to create stream buffer");
    }
    else
    {
        ESP_LOGI("STREAM_BUFFER", "Stream buffer created, size: %d bytes", STREAM_BUFFER_SIZE);
    }
}

size_t stream_buffer_write(const int16_t *data, size_t len)
{
    if (stream_buffer == NULL || data == NULL || len == 0)
    {
        return 0;
    }

    size_t bytes_written = xStreamBufferSend(stream_buffer, data, len * sizeof(int16_t), 0);
    return bytes_written / sizeof(int16_t);
}

size_t stream_buffer_read(int16_t *data, size_t len)
{
    if (stream_buffer == NULL || data == NULL || len == 0)
    {
        return 0;
    }

    size_t bytes_received = xStreamBufferReceive(stream_buffer, data, len * sizeof(int16_t), 0);
    return bytes_received / sizeof(int16_t);
}

size_t stream_buffer_get_free_size(void)
{
    if (stream_buffer == NULL)
    {
        return 0;
    }
    return xStreamBufferSpacesAvailable(stream_buffer) / sizeof(int16_t);
}

// Original ring buffer functions (keep existing implementation)
void ring_buffer_create(void)
{
    audio_buffer.buffer = heap_caps_aligned_alloc(32, RING_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (audio_buffer.buffer == NULL)
    {
        esp_rom_printf("Failed to allocate buffer");
        return;
    }
    audio_buffer.size = RING_BUFFER_SIZE;
    audio_buffer.write_ptr = audio_buffer.read_ptr = 0;
    audio_buffer.lock = xSemaphoreCreateBinary();
    xSemaphoreGive(audio_buffer.lock);
    esp_rom_printf("Buffer initialized: write_ptr=%d, read_ptr=%d, size=%d\n",
                   audio_buffer.write_ptr, audio_buffer.read_ptr, audio_buffer.size);
}

bool ring_buffer_write(const int16_t *data, size_t len)
{
    if (data == NULL || len == 0 || len > RING_BUFFER_SIZE)
    {
        esp_rom_printf("Invalid write parameters: data=%p, len=%d\n", data, len);
        return false;
    }

    bool ret = false;
    if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
    {
        size_t write_ptr = audio_buffer.write_ptr;
        size_t read_ptr = audio_buffer.read_ptr;
        size_t free_size;

        // Calculate free space in the buffer
        if (write_ptr >= read_ptr)
        {
            free_size = RING_BUFFER_SIZE - (write_ptr - read_ptr) - 1;
        }
        else
        {
            free_size = read_ptr - write_ptr - 1;
        }
        // Ensure we don't underflow
        if (free_size > RING_BUFFER_SIZE)
        {
            free_size = 0;
        }

        if (free_size >= len)
        {
            if (write_ptr + len > RING_BUFFER_SIZE)
            {
                // Write in two parts
                size_t first_part = RING_BUFFER_SIZE - write_ptr;
                memcpy(&audio_buffer.buffer[write_ptr], data, first_part * sizeof(int16_t));
                memcpy(audio_buffer.buffer, &data[first_part], (len - first_part) * sizeof(int16_t));
                audio_buffer.write_ptr = len - first_part;
            }
            else
            {
                // Write in one go
                memcpy(&audio_buffer.buffer[write_ptr], data, len * sizeof(int16_t));
                audio_buffer.write_ptr = (write_ptr + len) % RING_BUFFER_SIZE;
            }
            ret = true;
        }
        else
        {
            esp_rom_printf("Not enough space to write: free_size=%d, len=%d\n", free_size, len);
        }

        xSemaphoreGive(audio_buffer.lock);
    }
    else
    {
        esp_rom_printf("Failed to take semaphore for write\n");
    }

    return ret;
}

size_t ring_buffer_read(int16_t *data, size_t len)
{
    if (!data || len == 0)
    {
        return 0;
    }

    size_t read_len = 0;

    if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
    {
        // Calculate available data
        size_t read_pos = audio_buffer.read_ptr;
        size_t write_pos = audio_buffer.write_ptr;
        size_t available;

        if (write_pos == read_pos)
        {
            // Buffer empty
            available = 0;
        }
        else if (write_pos > read_pos)
        {
            available = write_pos - read_pos;
        }
        else
        {
            available = RING_BUFFER_SIZE - read_pos + write_pos;
        }

        // Limit read length to available data
        read_len = (len > available) ? available : len;

        if (read_len > 0)
        {
            // Read data in one or two parts
            if (read_pos + read_len <= RING_BUFFER_SIZE)
            {
                // Single read
                memcpy(data, &audio_buffer.buffer[read_pos], read_len * sizeof(int16_t));
                audio_buffer.read_ptr = (read_pos + read_len) % RING_BUFFER_SIZE;

                // Clear read data
                memset(&audio_buffer.buffer[read_pos], 0, read_len * sizeof(int16_t));
            }
            else
            {
                // Split read
                size_t first_part = RING_BUFFER_SIZE - read_pos;
                memcpy(data, &audio_buffer.buffer[read_pos], first_part * sizeof(int16_t));
                memcpy(&data[first_part], audio_buffer.buffer,
                       (read_len - first_part) * sizeof(int16_t));
                audio_buffer.read_ptr = read_len - first_part;

                // Clear read data
                memset(&audio_buffer.buffer[read_pos], 0, first_part * sizeof(int16_t));
                memset(audio_buffer.buffer, 0, (read_len - first_part) * sizeof(int16_t));
            }
        }

        xSemaphoreGive(audio_buffer.lock);
        // ESP_LOGI("RING_BUFFER", "Read %d samples, read_ptr=%d",read_len, audio_buffer.read_ptr);
    }

    return read_len;
}

size_t ring_buffer_get_free_size(void)
{
    size_t free_size;
    if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
    {
        size_t write_ptr = audio_buffer.write_ptr;
        size_t read_ptr = audio_buffer.read_ptr;

        if (write_ptr >= read_ptr)
        {
            free_size = audio_buffer.size - (write_ptr - read_ptr) - 1;
        }
        else
        {
            free_size = read_ptr - write_ptr - 1;
        }
        xSemaphoreGive(audio_buffer.lock);
    }
    else
    {
        ESP_LOGW(__func__, "Failed to take semaphore in get_free_size");
        free_size = 0;
    }
    return free_size;
}
