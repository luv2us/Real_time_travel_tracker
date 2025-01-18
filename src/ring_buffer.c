#include "ring_buffer.h"

ring_buffer_t audio_buffer;
// ????????��?????
void ring_buffer_create(void)
{
    audio_buffer.buffer = heap_caps_malloc(RING_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_INTERNAL);
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
            free_size = RING_BUFFER_SIZE - write_ptr + (read_ptr > 0 ? read_ptr - 1 : 0);
        }
        else
        {
            free_size = read_ptr - write_ptr - 1;
        }

        esp_rom_printf("Before write: write_ptr=%d, read_ptr=%d, free_size=%d, len=%d\n",
                       write_ptr, read_ptr, free_size, len);

        if (free_size >= len)
        {
            esp_rom_printf("Writing data...\n");
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
                for (int i = write_ptr; i < write_ptr + 20; i++)
                {
                    esp_rom_printf("%d,", audio_buffer.buffer[i]);
                }
                // esp_rom_printf("old=%d\n", audio_buffer.write_ptr);
                // esp_rom_printf("len =%d\r(write_ptr + len)%d\rnew=%d\n", len, (write_ptr + len), (write_ptr + len) % RING_BUFFER_SIZE);
                audio_buffer.write_ptr = (write_ptr + len) % RING_BUFFER_SIZE;
            }
            ret = true;
        }
        else
        {
            esp_rom_printf("Not enough space to write: free_size=%d, len=%d\n", free_size, len);
        }

        esp_rom_printf("After write: write_ptr=%d, read_ptr=%d\n",
                       audio_buffer.write_ptr, audio_buffer.read_ptr);

        xSemaphoreGive(audio_buffer.lock);
    }
    else
    {
        esp_rom_printf("Failed to take semaphore for write\n");
    }

    return ret;
}
// bool ring_buffer_write(const int16_t *data, size_t len)
// {
//     if (!data || len == 0 || len > RING_BUFFER_SIZE)
//     {
//         esp_rom_printf("no data or len is 0 or len is greater than buffer size\n");
//         return false;
//     }

//     bool ret = false;
//     if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
//     {
//         // 原子化获取当前指针位置
//         size_t write_ptr = audio_buffer.write_ptr;
//         size_t read_ptr = audio_buffer.read_ptr;
//         size_t free_size;

//         // 修正空闲空间计算
//         if (write_ptr >= read_ptr)
//         {
//             free_size = RING_BUFFER_SIZE - write_ptr;
//             if (read_ptr > 0)
//             {
//                 free_size += read_ptr - 1;
//             }
//         }
//         else
//         {
//             free_size = read_ptr - write_ptr - 1;
//         }

//         if (free_size >= len)
//         {
//             esp_rom_printf("trying to write\n");
//             if (write_ptr + len <= RING_BUFFER_SIZE)
//             {
//                 memcpy(&audio_buffer.buffer[write_ptr], data, len * sizeof(int16_t));
//                 audio_buffer.write_ptr = (write_ptr + len == RING_BUFFER_SIZE) ? 0 : write_ptr + len;
//             }
//             else
//             {
//                 size_t first_part = RING_BUFFER_SIZE - write_ptr;
//                 memcpy(&audio_buffer.buffer[write_ptr], data, first_part * sizeof(int16_t));
//                 memcpy(audio_buffer.buffer, &data[first_part], (len - first_part) * sizeof(int16_t));
//                 audio_buffer.write_ptr = len - first_part;
//             }
//             ret = true;
//             esp_rom_printf("write %dsize\n", len);
//         }
//         xSemaphoreGive(audio_buffer.lock);
//     }
//     return ret;
// }
// bool ring_buffer_write(const int16_t *data, size_t len)
// {
//     if (len == 0 || len > RING_BUFFER_SIZE)
//     {
//         return false;
//     }

//     if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
//     {
//         // ??????
//         size_t free_size;
//         if (audio_buffer.write_ptr >= audio_buffer.read_ptr)
//         {
//             free_size = RING_BUFFER_SIZE - (audio_buffer.write_ptr - audio_buffer.read_ptr);
//         }
//         else
//         {
//             free_size = audio_buffer.read_ptr - audio_buffer.write_ptr;
//         }

//         // ????????
//         if (free_size > len)
//         {
//             // ????
//             if (audio_buffer.write_ptr + len <= RING_BUFFER_SIZE)
//             {
//                 // ?????
//                 memcpy(&audio_buffer.buffer[audio_buffer.write_ptr],
//                        data, len * sizeof(int16_t));
//                 audio_buffer.write_ptr = (audio_buffer.write_ptr + len) % RING_BUFFER_SIZE;
//             }
//             else
//             {
//                 // ?????
//                 size_t first_part = RING_BUFFER_SIZE - audio_buffer.write_ptr;
//                 memcpy(&audio_buffer.buffer[audio_buffer.write_ptr],
//                        data, first_part * sizeof(int16_t));
//                 memcpy(audio_buffer.buffer,
//                        &data[first_part], (len - first_part) * sizeof(int16_t));
//                 audio_buffer.write_ptr = len - first_part;
//             }
//             xSemaphoreGive(audio_buffer.lock);
//             return true;
//         }
//         xSemaphoreGive(audio_buffer.lock);
//     }
//     /*  size_t next = (audio_buffer.write_ptr + len) % audio_buffer.size;
//     if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
//     {
//         esp_rom_printf("take semaphore success");
//         if (next == audio_buffer.read_ptr || (audio_buffer.write_ptr < audio_buffer.read_ptr && next >= audio_buffer.read_ptr))
//         {
//             memcpy(&audio_buffer.buffer[audio_buffer.write_ptr],
//                    data, len * sizeof(int16_t));
//             audio_buffer.write_ptr = next;
//             xSemaphoreGive(audio_buffer.lock);

//             return true;
//         }
//         xSemaphoreGive(audio_buffer.lock);
//     }
//     else
//     {
//         esp_rom_printf("take semaphore failed");
//     }*/

//     return false;
// }
// ???????????
size_t ring_buffer_read(int16_t *data, size_t len)
{
    if (!data || len == 0)
    {
        return 0;
    }

    size_t read_len = 0;

    if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
    {
        // ???????
        size_t read_pos = audio_buffer.read_ptr;
        size_t write_pos = audio_buffer.write_ptr;
        size_t available;

        if (write_pos >= read_pos)
        {
            available = write_pos - read_pos;
            esp_rom_printf(" write_pos >= read_pos\n");
        }
        else
        {
            esp_rom_printf(" write_pos < read_pos\n");
            available = RING_BUFFER_SIZE - read_pos + write_pos;
        }

        if (available == 0)
        {
            esp_rom_printf("read failed , No data available\n");
            xSemaphoreGive(audio_buffer.lock);
            return 0;
        }

        // ??????
        read_len = (len > available) ? available : len;

        // ????
        if (read_pos + read_len <= RING_BUFFER_SIZE)
        {
            // ?????
            memcpy(data, &audio_buffer.buffer[read_pos], read_len); //* sizeof(int16_t)
            audio_buffer.read_ptr = (read_pos + read_len) % RING_BUFFER_SIZE;
        }
        else
        {
            // ?????
            size_t first_part = RING_BUFFER_SIZE - read_pos;
            memcpy(data, &audio_buffer.buffer[read_pos], first_part); //* sizeof(int16_t)
            memcpy(&data[first_part], audio_buffer.buffer,
                   (read_len - first_part)); //* sizeof(int16_t)
            audio_buffer.read_ptr = read_len - first_part;
        }

        xSemaphoreGive(audio_buffer.lock);
        ESP_LOGI("RING_BUFFER", "Read %d bytes, read_ptr=%d",
                 read_len, audio_buffer.read_ptr);
    }

    return read_len;
}
// size_t ring_buffer_read(int16_t *data, size_t len)
// {
//     size_t count = 0;
//     esp_rom_printf("trying to read\n");
//     if (xSemaphoreTake(audio_buffer.lock, portMAX_DELAY))
//     {
//         size_t available = (audio_buffer.write_ptr - audio_buffer.read_ptr +
//                             audio_buffer.size) %
//                            audio_buffer.size;

//         // ?????????????????
//         count = (len > available) ? available : len;

//         if (count > 0)
//         {
//             // ??????????????????
//             memcpy(data, &audio_buffer.buffer[audio_buffer.read_ptr],
//                    count * sizeof(int16_t));
//             audio_buffer.read_ptr = (audio_buffer.read_ptr + count) %
//                                     audio_buffer.size;
//         }
//         else
//         {
//             ESP_LOGW(__func__, "ringbuffer No data\n");
//         }
//         xSemaphoreGive(audio_buffer.lock);
//     }

//     return count;
// }
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
