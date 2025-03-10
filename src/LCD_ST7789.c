#include "LCD_ST7789.h"
#include "esp_sleep.h"



spi_device_handle_t spi;

static bool isOpen = TRUE;
static bool isInit = FALSE;
static uint8_t Battery = 0;
static uint8_t lastBattery = 255;
static bool isHomepage = TRUE;
static bool isOnMenu1 = FALSE;
static bool isOnMenu2 = FALSE;
static bool isOnMenu3 = FALSE;
static uint8_t key_state = 0; // 按键状态
static int64_t previousMillis = 0;  // 上次执行的时间点
static int64_t currentMillis = 0;
static int64_t previousMillis_readBat = 0;
static int64_t interval = SCREEN_TIMEOUT_1;      // 初始化为30秒息屏
static bool is_on_menu3_page2 = FALSE;
static int64_t interval_readBat = 30000000; // 间隔时间（半分钟 = 30，000，000微妙）
static uint8_t menuChooseNum = 1;
static uint8_t lastMenuChooseNum = 1;
static bool isPressTalk = FALSE;
static bool isTalking = FALSE;
static bool lastChargeState = FALSE;
static bool chargeState = FALSE;
static bool firstTimeStopCharging = FALSE;
static bool isOnLowBatteryWarningPage = FALSE;
const uint8_t brightness_values[BRIGHTNESS_LEVELS] = {
    0, 26, 51, 77, 102, 128, 153, 179, 204, 230, 255
};
uint8_t currentBrightnessLevel = 10; // 0-10档
static int64_t press_power_time = 0;
static int64_t last_press_power_time = 0;
bool is_press_power = FALSE;
bool press_power_start = FALSE;
static uint16_t colorBuffer[LCD_WIDTH * LCD_HEIGHT] = {0};

static void WriteCommand(uint8_t cmd);
static void WriteData(uint8_t data);
static void write16Bits(uint16_t data);
static void write_multi_bytes(uint16_t *data, uint16_t size);
static void write_large_data(uint16_t *data, uint16_t totalNum);

static void SetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);// 窗口设置函数
static void ShowChinese32x32(uint16_t x,uint16_t y,char *s,uint16_t fc,uint8_t sizey);
static uint32_t mypow(uint8_t m,uint8_t n);
static void Get_GBK_Address(char *code,uint8_t *character); // 获取GBK码对应的地址
static void DrawPoint(uint32_t xStart, uint32_t yStart, uint32_t color);        // 画点
static void DrawLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color); //画线
static void DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color);//画矩形
//static void SetBGColor(uint32_t color);// 设置背景色
static void FillBlock(uint16_t xStart, uint16_t yStart, uint16_t block_width, uint16_t block_height, uint16_t color);// 填充块
//static void DrawPic(uint32_t xStart, uint32_t yStart, uint32_t block_width, uint32_t block_height, const uint8_t *block_data);//绘制图片
static void DrawImageBin(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t *image_mask, uint32_t color, bool reverse);// 二进制制图
//static void DrawCycle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t forecolor, uint8_t mask_quadrant);//画圆
static void DrawFilledCycle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color, uint8_t mask_quadrant);// 画圆块
static void ShowChinese(uint16_t x,uint16_t y,char *s,uint16_t fc,uint8_t sizey); // 显示中文字段
static void ShowChar(uint16_t x,uint16_t y,uint8_t num,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);    // 显示字符
static void ShowString(uint16_t x,uint16_t y,const char *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode); // 显示字符串
static void ShowIntNum(uint16_t x,uint16_t y,uint16_t num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode); // 显示数字
static void drawGradientBackground();  //渐变背景
static void clearRectToGradient(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height); // 清除矩形区域并使用渐变填充
static void clear();
static void DisplayBattery(); // 显示电池电量
static void GPS(bool state); // GPS打开
static void Microphone(bool state); // 显示麦克风
static void name(bool state); // 显示名称
static void setName(char *s); // 设置名称
static void menu(bool state); // 显示菜单
static void loraSignal(uint8_t state); // LoRa信道信号
static void chooseMenu(uint8_t state); // 选择菜单
static void voiceMenu(bool state); // 通话方式控制菜单
static void chargeIconDisplay(bool state); // 充电图标显示
static void changeBtaIconDisplay(); // 是否显示充电图标
static bool isCharging(); // 是否在充电
static void menu1();
static void menu2();
static void menu3();
static void pwm_init();
static void set_backlight_brightness(uint8_t brightness);
static void closeBacklight(bool enable);
//static void BlueTooth(bool state);  // 蓝牙打开
static void increase_brightness();
static void decrease_brightness();


static void WriteCommand(uint8_t cmd) {
    LCD_writeCMD;  
    LCD_CS_START;  
    uint8_t tx_data = cmd;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &tx_data,
        .rx_buffer = NULL
    };
    spi_device_polling_transmit(spi, &t);
    LCD_CS_STOP; 
}
    
static void WriteData(uint8_t data) {
    LCD_writeData; 
    LCD_CS_START;

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    ret = spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);

    LCD_CS_STOP;
}

static void write16Bits(uint16_t data) {
    LCD_writeData; 
    LCD_CS_START;   

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint8_t tx_data[2] = {(uint8_t)(data >> 8), (uint8_t)data};

    t.length = 8 * 2;
    t.tx_buffer = tx_data;
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret==ESP_OK);

    LCD_CS_STOP;
}

static void write_multi_bytes(uint16_t *data, uint16_t dataNum){
    LCD_writeData; 
    LCD_CS_START;

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint8_t tx_buffer[dataNum * 2];

    for (uint16_t i = 0; i < dataNum; i++) {
        tx_buffer[i * 2]     = (uint8_t)(data[i] >> 8);  // 高字节
        tx_buffer[i * 2 + 1] = (uint8_t)(data[i] & 0xFF); // 低字节
    }

    t.length = 8 * dataNum * 2;
    t.tx_buffer = tx_buffer;
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret==ESP_OK);

    LCD_CS_STOP;
}

static void write_large_data(uint16_t *data, uint16_t totalNum) {
    const uint16_t CHUNK_SIZE = 512;  // 最大 DMA 传输数量
    uint16_t remaining = totalNum;
    uint16_t *ptr = data;

    while (remaining > 0) {
        uint16_t chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

        write_multi_bytes(ptr, chunk);  // 发送一块数据
        ptr += chunk;
        remaining -= chunk;
    }
}


    // 窗口设置函数
    // 坐标都从1开始，即1-240和1-135
static void SetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd){
    WriteCommand(0x2a);
    write16Bits(xStart + COL_OFFSET - 1);
    write16Bits(xEnd + COL_OFFSET - 1);
    
    WriteCommand(0x2b);
    write16Bits(yStart + ROW_OFFSET - 1);
    write16Bits(yEnd + ROW_OFFSET - 1);
    
    WriteCommand(0x2c);
}
    
void LCD_init() {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_40M,  // SPI时钟频率
        .mode = 0,                  // SPI模式0
        .spics_io_num = -1,         // 不使用CS引脚
        .queue_size = 1,            // 传输队列大小
    };
    
    // 添加SPI设备
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
    key_init();
    
    // 初始化状态变量
    isOpen = TRUE;
    isInit = FALSE;
    isHomepage = TRUE;
    isOnMenu1 = FALSE;
    isOnMenu2 = FALSE;
    isOnMenu3 = FALSE;
    isPressTalk = FALSE;
    lastChargeState = FALSE;
    chargeState = FALSE;
    firstTimeStopCharging = FALSE;
    isOnLowBatteryWarningPage = FALSE;
    // 配置引脚
    gpio_set_direction(LEDA, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_RS, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_SS1, GPIO_MODE_OUTPUT);
    gpio_set_direction(TFT_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHRG_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(CHRG_PIN);
    LCD_CS_STOP;
    
    
    // 初始化显示屏
    LCD_RESET_START;       // 复位显示屏
    vTaskDelay(pdMS_TO_TICKS(200));            // 等待复位完成
    LCD_RESET_STOP;
    vTaskDelay(pdMS_TO_TICKS(100));
    WriteCommand(0x11);    // 退出睡眠模式
    vTaskDelay(pdMS_TO_TICKS(10));
    
    WriteCommand(0x3A);    // 设置颜色模式  Interface Pixel Format 
    WriteData(0x05);       // RGB565格式 (16位/像素)
    
    // //与屏的型号有关
    WriteCommand(0x21);    //开启反转显示，否者显示为反色
	WriteCommand(0xb2);
	WriteData(0x0c); 
	WriteData(0x0c); 
	WriteData(0x00); 
	WriteData(0x33);
	WriteData(0x33);
	 
	WriteCommand(0xb7);
	WriteData(0x35);
    
	WriteCommand(0xbb);
	WriteData(0x35);
	WriteCommand(0xc0);
	WriteData(0x2c);
	WriteCommand(0xc2);
	WriteData(0x01);
	WriteCommand(0xc3);
	WriteData(0x0b);
	WriteCommand(0xc4);
	WriteData(0x20);
	WriteCommand(0xc6);
	WriteData(0x1e);
	WriteCommand(0xd0);
	WriteData(0xa4);
	WriteData(0xa1);
    
	WriteCommand(0xE0);    
	WriteData(0xd0);
	WriteData(0x00);
	WriteData(0x02);
	WriteData(0x07);
	WriteData(0x0b);
	WriteData(0x1a);
	WriteData(0x31);
	WriteData(0x54);
	WriteData(0x40);
	WriteData(0x29);
	WriteData(0x12);
	WriteData(0x12);
	WriteData(0x12);
	WriteData(0x17);
	
	WriteCommand(0XE1);    
	WriteData(0xd0);
	WriteData(0x00);
	WriteData(0x02);
	WriteData(0x07);
	WriteData(0x05);
	WriteData(0x25);
	WriteData(0x2d);
	WriteData(0x44);
	WriteData(0x45);
	WriteData(0x1c);
	WriteData(0x18);
	WriteData(0x16);
	WriteData(0x1c);
	WriteData(0x1d);
    WriteCommand(0x36);    // 设置显示方向
    WriteData(0x60);       // 旋转90度
	WriteCommand(0x29);    // 打开显示
    
    drawGradientBackground(); 
    name(OPEN);

    GPS(OPEN);
    loraSignal(4);
    DisplayBattery();

    pwm_init(); // PWM控制背光
    vTaskDelay(pdMS_TO_TICKS(10));           // 等待背光稳定
}
    

// 画点函数，在坐标为(xStart,yStart)的位置，画一个点
// 包块setwindow函数，坐标都从1开始，即1-240和1-135
static void DrawPoint(uint32_t xStart, uint32_t yStart, uint32_t color) 
{
    WriteCommand(0x2a);
    write16Bits(xStart + COL_OFFSET - 1);
    
    WriteCommand(0x2b);
    write16Bits(yStart + ROW_OFFSET - 1);
    
    WriteCommand(0x2c);
    write16Bits(color);
}
/******************************************************************************
  函数说明：画线
  入口数据：x1,y1   起始坐标
            x2,y2   终止坐标
            color   线的颜色
  返回值：  无
******************************************************************************/
static void DrawLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{
    uint16_t t; 
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    delta_x=x2-x1; //计算坐标增量 
    delta_y=y2-y1;
    uRow=x1;//画线起点坐标
    uCol=y1;
    if(delta_x>0)incx=1; //设置单步方向 
    else if (delta_x==0)incx=0;//垂直线 
    else {incx=-1;delta_x=-delta_x;}
    if(delta_y>0)incy=1;
    else if (delta_y==0)incy=0;//水平线 
    else {incy=-1;delta_y=-delta_y;}
    if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
    else distance=delta_y;
    for(t=0;t<distance+1;t++)
    {
        DrawPoint(uRow,uCol,color);//画点
        xerr+=delta_x;
        yerr+=delta_y;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}
/******************************************************************************
  函数说明：画矩形
  入口数据：x1,y1   起始坐标
            x2,y2   终止坐标
            color   矩形的颜色
  返回值：  无
******************************************************************************/
static void DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color)
{
    DrawLine(x1,y1,x2,y1,color);
    DrawLine(x1,y1,x1,y2,color);
    DrawLine(x1,y2,x2,y2,color);
    DrawLine(x2,y1,x2,y2,color);
}
// 设置背景色
// static void SetBGColor(uint32_t color)
// {
//     uint32_t i,j;
    
//     SetWindow(0,0,(LCD_WIDTH-1),(LCD_HEIGHT-1));
    
//     for(i=0;i<LCD_HEIGHT;i++)
//     {
//         for(j=0;j<LCD_WIDTH;j++)
//         {
//             write16Bits(color);
//         }
//     }
// }
// 填充块
static void FillBlock(uint16_t xStart, uint16_t yStart, uint16_t block_width, uint16_t block_height, uint16_t color)
{
    if (block_width == 0 || block_height == 0)
    {
        return;
    }


    SetWindow(xStart, yStart, (xStart + block_width - 1), (yStart + block_height - 1));
    
    // 检查是否超出数组大小
    uint32_t totalPixels = block_width * block_height;
    if (totalPixels > LCD_WIDTH * LCD_HEIGHT) {
        printf("Error: Block size exceeds buffer capacity\n");
        return;
    }
    
    // 填充颜色数据
    for (uint32_t i = 0; i < totalPixels; i++) {
        colorBuffer[i] = color;
    }
    
    // 使用静态数组替代动态分配的内存
    write_large_data(colorBuffer, totalPixels);
}
//在坐标为(xStart,yStart)的位置，
//填充一个宽和高分别为block_width、block_height的图片
//block_data为指向图片点阵数据的指针，
//图片点阵数据格式为 RGB:565
//备注：高位先行
// static void DrawPic(uint32_t xStart, uint32_t yStart, uint32_t block_width, uint32_t block_height, const uint8_t *block_data)
// {
//     uint32_t i,j;

//     SetWindow( xStart, yStart, (xStart+block_width-1), (yStart+block_height-1) );
    
//     for(i=0;i<block_width;i++)
//     {
//         for(j=0;j<block_height;j++)
//         {
//             WriteData(*block_data);
//             WriteData(*(block_data+1));
            
//             block_data+=2;
//         }
//     }
// }
// 二进制制图
static void DrawImageBin(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t *image_mask, uint32_t color, bool reverse)
{
    uint32_t i, j;
    uint32_t x0, y0;
    uint8_t  bit_mask;
    
    for(i = 0; i < h; i++)
    {
        y0 = y + i;
        
        bit_mask = 0x80;
        for(j = 0; j < w; j++)
        {
            if(reverse)
                x0 = x + w - 1 - j;
            else
                x0 = x + j;

            if(*image_mask & bit_mask)
            {
                DrawPoint(x0, y0, color);
            }

            bit_mask >>= 1;
            if(bit_mask == 0)
            {
                bit_mask = 0x80;
                image_mask++;
            }
        }
        if(bit_mask != 0x80)
            image_mask++;        
    }
}
// 画一个圆形（无填充）
// x0,y0	      圆心坐标
// r              圆半径
// mask_quadrant  象限选择
// 0x01,0x02,0x04,0x08
// static void DrawCycle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t forecolor, uint8_t mask_quadrant)
// {
//     int a, b;
//     int di;
    
//     a = 0;
//     b = r;
//     di = 3 - (r << 1);

//     while(a <= b)
//     {
//         // 第 1 象限
//         if(mask_quadrant & 0x01)
//         {
//             DrawPoint(x0 + b, y0 - a, forecolor);
//             DrawPoint(x0 + a, y0 - b, forecolor);
//         }
        
//         // 第 2 象限
//         if(mask_quadrant & 0x02)
//         {
//             DrawPoint(x0 - b, y0 - a, forecolor);
//             DrawPoint(x0 - a, y0 - b, forecolor);
//         }
        
//         // 第 3 象限
//         if(mask_quadrant & 0x04)
//         {
//             DrawPoint(x0 - a, y0 + b, forecolor);
//             DrawPoint(x0 - b, y0 + a, forecolor);
//         }
        
//         // 第 4 象限
//         if(mask_quadrant & 0x08)
//         {
//             DrawPoint(x0 + b, y0 + a, forecolor);
//             DrawPoint(x0 + a, y0 + b, forecolor);
//         }
            
//         a++;

//         if(di < 0)
//         {
//             di += 4 * a + 6;
//         }
//         else
//         {
//             di += 10 + 4 * (a - b);
//             b--;
//         }
//     }
// }
// 画一个圆块
// x0,y0	      圆心坐标
// r              圆半径
// color          颜色
// mask_quadrant  象限选择
// 0x01,0x02,0x04,0x08
static void DrawFilledCycle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color, uint8_t mask_quadrant)
{
    int16_t sCurrentX, sCurrentY;
    int16_t sError;

    sCurrentX = 0;
    sCurrentY = r;	
    sError    = 3- (r<<1);   //判断下个点位置的标志
    
    while(sCurrentX <= sCurrentY)
    {
        int16_t sCountY;		
            
        for(sCountY=sCurrentX;sCountY<=sCurrentY;sCountY++)
        {                      
            // 第1象限 (x+,y-)
            if(mask_quadrant & 0x01) {
                DrawPoint(x0 + sCurrentX, y0 - sCountY, color);
                DrawPoint(x0 + sCountY, y0 - sCurrentX, color);
            }
            
            // 第2象限 (x-,y-)
            if(mask_quadrant & 0x02) {
                DrawPoint(x0 - sCurrentX, y0 - sCountY, color);
                DrawPoint(x0 - sCountY, y0 - sCurrentX, color);
            }
            
            // 第3象限 (x-,y+)
            if(mask_quadrant & 0x04) {
                DrawPoint(x0 - sCurrentX, y0 + sCountY, color);
                DrawPoint(x0 - sCountY, y0 + sCurrentX, color);
            }
            
            // 第4象限 (x+,y+)
            if(mask_quadrant & 0x08) {
                DrawPoint(x0 + sCurrentX, y0 + sCountY, color);
                DrawPoint(x0 + sCountY, y0 + sCurrentX, color);
            }
        }
            
        sCurrentX++;		
        if(sError < 0) 
        {
            sError += (4*sCurrentX + 6);	  
        }
        else
        {
            sError += (10 + 4*(sCurrentX - sCurrentY));   
            sCurrentY--;
        } 
    }
}

/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示坐标
                *s 要显示的汉字串
                fc 字的颜色
                bc 字的背景色
                sizey 字号只可选32
      返回值：  无

      备注：取模选择阴码,逆向，逐行式
******************************************************************************/
static void ShowChinese(uint16_t x,uint16_t y,char *s,uint16_t fc,uint8_t sizey)
{   
	while(*s!=0)
	{
		// if(sizey==12) ShowChinese12x12(x,y,s,fc,bc,sizey,mode);
		// else if(sizey==16) ShowChinese16x16(x,y,s,fc,bc,sizey,mode);
		// else if(sizey==24) ShowChinese24x24(x,y,s,fc,bc,sizey,mode);
		if(sizey==32) ShowChinese32x32(x,y,s,fc,sizey);
		else return;
		s+=2;
		x+=sizey;
	}
}

#if 0
/******************************************************************************
      函数说明：显示单个12x12汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void ShowChinese12x12(uint16_t x,uint16_t y,const char *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
{
	uint8_t i,j,m=0;
	uint16_t k;
	uint16_t HZnum;//汉字数目
	uint16_t TypefaceNum;//一个字符所占字节大小
	uint16_t x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	                         
	HZnum=sizeof(tfont12)/sizeof(typFNT_12);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
		{ 	
			SetWindow(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
					if(!mode)//非叠加方式
					{
						if(tfont12[k].Msk[i]&(0x01<<j))write16Bits(fc);
						else write16Bits(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont12[k].Msk[i]&(0x01<<j))	DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
} 

/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  MODE_NON_OVERLAPPING非叠加模式  MODE_OVERLAPPING叠加模式
      返回值：  无
******************************************************************************/
void ShowChinese16x16(uint16_t x,uint16_t y,const char *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
{
	uint8_t i,j,m=0;
	uint16_t k;
	uint16_t HZnum;//汉字数目
	uint16_t TypefaceNum;//一个字符所占字节大小
	uint16_t x0=x;
    TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont16)/sizeof(typFNT_16);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
		{ 	
			SetWindow(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
					if(!mode)//非叠加方式
					{
						if(tfont16[k].Msk[i]&(0x01<<j))write16Bits(fc);
						else write16Bits(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont16[k].Msk[i]&(0x01<<j))	DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
} 


/******************************************************************************
      函数说明：显示单个24x24汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  MODE_NON_OVERLAPPING非叠加模式  MODE_OVERLAPPING叠加模式
      返回值：  无
******************************************************************************/
void ShowChinese24x24(uint16_t x,uint16_t y,const char *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
{
	uint8_t i,j,m=0;
	uint16_t k;
	uint16_t HZnum;//汉字数目
	uint16_t TypefaceNum;//一个字符所占字节大小
	uint16_t x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont24)/sizeof(typFNT_24);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
		{ 	
			SetWindow(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
					if(!mode)//非叠加方式
					{
						if(tfont24[k].Msk[i]&(0x01<<j))write16Bits(fc);
						else write16Bits(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont24[k].Msk[i]&(0x01<<j))	DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
} 
#endif

/******************************************************************************
      函数说明：显示单个32x32汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
static void ShowChinese32x32(uint16_t x,uint16_t y,char *s,uint16_t fc,uint8_t sizey)
{   
	uint8_t i,j=0;
	uint16_t TypefaceNum;//一个中文字所占字节大小
	uint16_t x0=x;
    uint8_t character[128];
    Get_GBK_Address(s,character);
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	
	SetWindow(x,y,x+sizey-1,y+sizey-1);
	for(i=0;i<TypefaceNum;i++)
	{
		for(j=0;j<8;j++)
		{	
			if(character[i]&(0x80>>j))	DrawPoint(x,y,fc);//画一个点
			x++;
			if((x-x0)==sizey)
			{
				x=x0;
				y++;
				break;
			}
			
		}
	}
				  	
}		
	

static void Get_GBK_Address(char *code,uint8_t *character){
    uint8_t GBKH,GBKL;                 
    uint32_t offset;
    uint8_t buffer[128]; 

    GBKH=code[0];
    GBKL=code[1];

    if(GBKH>0xFE||GBKH<0x81){
        return;
    }
    GBKH-=0x81;
    GBKL-=0x40;

    //获取字符在bin文件中的偏移量，一个字符占(32*32)/8=128字节。
    offset=((uint32_t)192*GBKH+GBKL)*128;

    // 找到分区
    const esp_partition_t* ziku_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, ZIKU_PARTITION_LABEL);
    if (!ziku_partition) {
        printf("Failed to find hz partition!");
        return;
    }

    // 读取数据
    esp_partition_read(ziku_partition, offset, buffer, sizeof(buffer));

    memcpy(character, buffer, 128);
        
}

#if 0
size_t utf8_char_len(unsigned char c)
{
    if ((c & 0x80) == 0) {
        // 0xxxxxxx (ASCII)
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 110xxxxx
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        // 1110xxxx
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        // 11110xxx
        return 4;
    }
    // 如果遇到无法识别的，就当作单字节处理
    return 1;
}

char* utf8_split_chars(const char* s)
{
    if (s == NULL) {
        return NULL;
    }

    // 计算原串的字节数
    size_t input_len = strlen(s);
    Serial.print("input_len:");
    Serial.println(input_len);
    if (input_len == 0) {
        // 空串就直接返回一个空的动态分配字符串
        char* empty = (char*)malloc(1);
        if (empty) {
            *empty = '\0';
        }
        return empty;
    }

    // 默认中文字都为3字节编码(大多数中文字)
    char* result = (char*)malloc((input_len/3)*4);
    if (!result) {
        return NULL;  // 分配失败
    }

    size_t i = 0; // 读取 s 的指针
    size_t j = 0; // 写入 result 的指针

    while (i < input_len) {
        // 只处理 3 字节编码的 UTF-8 
        size_t c_len = utf8_char_len((unsigned char)s[i]);
        if(c_len != 3){
            return NULL;  // 字符不为3字节编码
        }
        
        // 将这 c_len 个字节（即一个完整 UTF-8 字符）拷贝到 result
        memcpy(&result[j], &s[i], c_len);
        j += c_len;

        // 在这个字符后面插入 '\0'
        result[j] = '\0';
        j++;

        // 跳过已经处理的 UTF-8 字符
        i += c_len;
    }

    // 若需要保证整个 result 最末也有一个 '\0' 作额外保险，可再执行：
    // （但实际上每个字符后面都已经放了 '\0'，因此这里可要可不要）
    // result[j] = '\0';

    return result;
}
#endif
/******************************************************************************
      函数说明：显示单个字符
      入口数据：x,y显示坐标
                num 要显示的字符
                fc 字的颜色
                bc 字的背景色
                sizey 字号 12,16,24,32
                mode:  MODE_NON_OVERLAPPING非叠加模式  MODE_OVERLAPPING叠加模式
      返回值：  无
******************************************************************************/
static void ShowChar(uint16_t x,uint16_t y,uint8_t num,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
{
	uint8_t temp,sizex,t,m=0;
	uint16_t i,TypefaceNum;//一个字符所占字节大小
	uint16_t x0=x;
	sizex=sizey/2;
	TypefaceNum=(sizex/8+((sizex%8)?1:0))*sizey;
	num=num-' ';    //得到偏移后的值
	SetWindow(x,y,x+sizex-1,y+sizey-1);  //设置光标位置 
	for(i=0;i<TypefaceNum;i++)
	{ 
		if(sizey==12)temp=ascii_1206[num][i];		       //调用6x12字体
		else if(sizey==16)temp=ascii_1608[num][i];		 //调用8x16字体
		else if(sizey==24)temp=ascii_2412[num][i];		 //调用12x24字体
		else if(sizey==32)temp=ascii_3216[num][i];		 //调用16x32字体
		else return;
		for(t=0;t<8;t++)
		{
			if(!mode)//非叠加模式
			{
				if(temp&(0x01<<t))write16Bits(fc);
				else write16Bits(bc);
				m++;
				if(m%sizex==0)
				{
					m=0;
					break;
				}
			}
			else//叠加模式
			{
				if(temp&(0x01<<t))DrawPoint(x,y,fc);//画一个点
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y++;
					break;
				}
			}
		}
	}   	 	  
}


/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y显示坐标
                *p 要显示的字符串
                fc 字的颜色
                bc 字的背景色
                sizey 字号 12:6x12  16:8x16  24:12x24  32:16x32
                mode:  MODE_NON_OVERLAPPING非叠加模式  MODE_OVERLAPPING叠加模式
      返回值：  无
******************************************************************************/
static void ShowString(uint16_t x,uint16_t y,const char *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
{         
	while(*p!='\0')
	{       
		ShowChar(x,y,*p,fc,bc,sizey,mode);
		x+=sizey/2;
		p++;
	}  
}


/******************************************************************************
      函数说明：显示数字
      入口数据：m底数，n指数
      返回值：  无
******************************************************************************/
static uint32_t mypow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;
	return result;
}


/******************************************************************************
      函数说明：显示整数变量
      入口数据：x,y显示坐标
                num 要显示整数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  MODE_NON_OVERLAPPING非叠加模式  MODE_OVERLAPPING叠加模式
      返回值：  无
******************************************************************************/
static void ShowIntNum(uint16_t x,uint16_t y,uint16_t num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;
	uint8_t sizex=sizey/2;
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				ShowChar(x+t*sizex,y,' ',fc,bc,sizey,mode);
				continue;
			}else enshow=1; 
		 	 
		}
	 	ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,mode);
	}
} 


// 在屏幕上绘制渐变背景
// startColor为起始颜色，endColor为结束颜色
static void drawGradientBackground() {

    // 提取起始颜色的 RGB565 分量
    uint16_t startR = (startColor >> 11) & 0x1F; // 红色 5 位
    uint16_t startG = (startColor >> 5) & 0x3F;  // 绿色 6 位
    uint16_t startB = startColor & 0x1F;         // 蓝色 5 位

    // 提取结束颜色的 RGB565 分量
    uint16_t endR = (endColor >> 11) & 0x1F; // 红色 5 位
    uint16_t endG = (endColor >> 5) & 0x3F;  // 绿色 6 位
    uint16_t endB = endColor & 0x1F;         // 蓝色 5 位


    for (uint16_t y = 0; y < LCD_HEIGHT; y++)
    {
        for (uint16_t x = 0; x < LCD_WIDTH; x++)
        {

            uint16_t ratio = (x * 255) / (LCD_WIDTH - 1);

            uint16_t r = startR + ((endR - startR) * ratio) / 255;
            uint16_t g = startG + ((endG - startG) * ratio) / 255;
            uint16_t b = startB + ((endB - startB) * ratio) / 255;

            colorBuffer[y * LCD_WIDTH + x] = (r << 11) | (g << 5) | b;
        }
    }

    SetWindow(1, 1, LCD_WIDTH, LCD_HEIGHT);
    write_large_data(colorBuffer, LCD_WIDTH * LCD_HEIGHT);
}

static void clearRectToGradient(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height)
{
    // 与 drawGradientBackground 相同的取色公式，需要同样的 startColor 和 endColor
    uint16_t startR = (startColor >> 11) & 0x1F;
    uint16_t startG = (startColor >> 5) & 0x3F;
    uint16_t startB = startColor & 0x1F;

    uint16_t endR = (endColor >> 11) & 0x1F;
    uint16_t endG = (endColor >> 5) & 0x3F;
    uint16_t endB = endColor & 0x1F;

    // 防止越界
    uint16_t xMax = x0 + width - 1;
    if (xMax > LCD_WIDTH)
        xMax = LCD_WIDTH;
    uint16_t yMax = y0 + height - 1;
    if (yMax > LCD_HEIGHT)
        yMax = LCD_HEIGHT;


    for (uint16_t y = y0; y <= yMax; y++)
    {
        for (uint16_t x = x0; x <= xMax; x++)
        {

            float ratio = (float)(x - 1) / (float)LCD_WIDTH;

            uint16_t r = startR + ratio * (endR - startR);
            uint16_t g = startG + ratio * (endG - startG);
            uint16_t b = startB + ratio * (endB - startB);

            colorBuffer[(y - y0) * (xMax - x0 + 1) + (x - x0)] = (r << 11) | (g << 5) | b;
        }
    }
    SetWindow(x0, y0, xMax, yMax);
    write_large_data(colorBuffer, (xMax - x0 + 1) * (yMax - y0 + 1));
}

static void clear(){
    clearRectToGradient(1,21,240,115);
}

static void DisplayBattery(){
    
    if(isInit == FALSE){
        // 绘制电池图标
        DrawLine(203, 18, 203, 5, COLOR_WHITE);
        DrawPoint(204,4,COLOR_WHITE);
        DrawLine(205, 3, 236, 3, COLOR_WHITE);
        DrawPoint(237,4,COLOR_WHITE);
        DrawLine(238, 5, 238, 18, COLOR_WHITE);
        DrawPoint(237,19,COLOR_WHITE);
        DrawLine(236, 20, 205, 20, COLOR_WHITE);
        DrawPoint(204,19,COLOR_WHITE);

        isInit = TRUE;

        if(isCharging()){
            #ifndef BEST

            clearRectToGradient(206,5, 30, 14); 
            FillBlock(206,5,3,14,COLOR_RED);

            #endif
            return;
        }
        
    }

    if(chargeState == TRUE)
        return;

    
    double voltage = ((double)battery_voltage * 1.5) / 1000.0; // 转换为伏特
    //clearRectToGradient(30,5, 32, 16);
    //ShowIntNum(30,5,(battery_voltage/2)*3,4,COLOR_WHITE,COLOR_BLACK,16,MODE_OVERLAPPING);
    printf("%lld ms - 电压: %.3f V\n", (long long)esp_timer_get_time() / 1000, voltage);

    if(voltage >= 3.9) {
        Battery = 5;
    }
    else if(voltage >= 3.7){
        Battery = 4;
    }
    else if(voltage >= 3.5) {
        Battery = 3;
    }
    else if(voltage >= 3.4) {
        Battery = 2;  
    }
    else if(voltage >= 3.3) {
        Battery = 1;  
    }
    else {
        Battery = 0; 
    }
    
    
    if((Battery >= lastBattery) && (chargeState == false) && (firstTimeStopCharging == FALSE))
        return;

    
    char s[] = { 0xB5,0xE7,0xC1,0xBF,0xB5,0xCD,0xA3,0xAC,0xC7,0xEB,0xB3,0xE4,0xB5,0xE7, '\0' }; // "电量低，请充电"
    // 显示电量
    switch(Battery){
        case 5:
            clearRectToGradient(206,5, 30, 14); // 清除电量显示区域
            FillBlock(206,5,6,14,COLOR_WHITE);
            FillBlock(214,5,6,14,COLOR_WHITE);
            FillBlock(222,5,6,14,COLOR_WHITE);
            FillBlock(230,5,6,14,COLOR_WHITE);
            break;
        case 4:
            clearRectToGradient(206,5, 30, 14); 
            FillBlock(206,5,6,14,COLOR_WHITE);
            FillBlock(214,5,6,14,COLOR_WHITE);
            FillBlock(222,5,6,14,COLOR_WHITE);
            break;
        case 3:
            clearRectToGradient(206,5, 30, 14); 
            FillBlock(206,5,6,14,COLOR_WHITE);
            FillBlock(214,5,6,14,COLOR_WHITE);
            break;
        case 2:
            clearRectToGradient(206,5, 30, 14); 
            FillBlock(206,5,6,14,COLOR_WHITE);
            break;
        case 1:
            clearRectToGradient(206,5, 30, 14); 
            FillBlock(206,5,3,14,COLOR_RED);
            isOnLowBatteryWarningPage = TRUE;
            clear();
            ShowChinese(10, 53, s, COLOR_RED, 32);
            if(isOpen == FALSE){
                LCD_BACKLIGHT_START;
                isOpen = TRUE;
            }
            previousMillis = esp_timer_get_time();
            break;
        case 0:
            clearRectToGradient(206,5, 30, 14); 
            FillBlock(206,5,3,14,COLOR_RED);

            #ifdef BEST
            // 电池电量过低，会出bug
            // 准备休眠
            LCD_BACKLIGHT_STOP;  // 关闭背光
            
            // 配置唤醒源 (充电时唤醒)
            esp_err_t ret = esp_sleep_enable_ext0_wakeup(CHRG_PIN, 0);  // GPIO6, 低电平唤醒
            if (ret != ESP_OK) {
                printf("配置唤醒源失败: %s\n", esp_err_to_name(ret));
                return;
            }
            
            // 进入轻度休眠
            ret = esp_light_sleep_start();
            if (ret != ESP_OK) {
                printf("进入轻度休眠失败: %s\n", esp_err_to_name(ret));
                return;
            }

            // 从这里恢复后需要重新初始化一些外设
            esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
            if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
                printf("被GPIO唤醒\n");
            }

            LCD_BACKLIGHT_START;  // 打开背光

            #endif

            break;
        default:
            printf("Battery:Error");
            break;
    }

    lastBattery = Battery;
    firstTimeStopCharging = FALSE;
}

static void chargeIconDisplay(bool state){
    if(state){
        DrawImageBin(210, 2, 16, 16, Image_charge, COLOR_GREEN, false);
    }else{
        clearRectToGradient(210,4,16,16);
    }
}

static void changeBtaIconDisplay() {
    /*   **************************************************           后续要注释掉            *****************************************************************          */
    #ifndef BEST
    double voltage = ((double)battery_voltage * 1.5) / 1000.0;
    if(voltage < 2.0)
        return;
    /*   **************************************************           后续要注释掉            *****************************************************************          */
    #endif

    chargeState = isCharging();
    if(chargeState != lastChargeState){
        if(isOpen == FALSE){
            LCD_BACKLIGHT_START;  
            isOpen = TRUE;
            previousMillis = currentMillis;
        }

        if(chargeState == TRUE){
            clearRectToGradient(206,5, 30, 14); // 清除电量显示区域
            chargeIconDisplay(OPEN);
            lastChargeState = chargeState;
        }
        else if(chargeState == FALSE){
            firstTimeStopCharging = TRUE;
            vTaskDelay(pdMS_TO_TICKS(2000));
            chargeIconDisplay(CLOSE);
            DisplayBattery();
            lastChargeState = chargeState;
        }
        previousMillis = currentMillis;
    }
    
}

static bool isCharging() {
    bool charging = (gpio_get_level(CHRG_PIN) == LOW);
    return charging;
}

// static void BlueTooth(bool state){
//     if(state){
//         DrawImageBin(184, 4, 16, 16, Image_BT, COLOR_BLUE, false);
//     }else{
//         clearRectToGradient(184,4,16,16);
//     }
// }

static void GPS(bool state){
    if(state){
        DrawImageBin(164, 4, 16, 16, Image_GPS, 0xFD8C, false);  
    }else{
        clearRectToGradient(164, 4, 16, 16);
    }
    
}

static void Microphone(bool state) {
    if(state){
        DrawImageBin(35, 4, 16, 16, Image_mic, COLOR_GREEN, false);  
    }else{
        clearRectToGradient(35, 4, 16, 16);
    }
}

static void loraSignal(uint8_t state){
    switch (state) {
        case 1:
            clearRectToGradient(3, 4, 23, 17);
            FillBlock(3, 16, 5, 4, COLOR_WHITE);  
            break;   
        case 2:
            clearRectToGradient(3, 4, 23, 17);
            FillBlock(3, 16, 5, 4, COLOR_WHITE);
            FillBlock(9, 12, 5, 8, COLOR_WHITE); 
            break;
        case 3:
            clearRectToGradient(3, 4, 23, 17);
            FillBlock(3, 16, 5, 4, COLOR_WHITE);
            FillBlock(9, 12, 5, 8, COLOR_WHITE); 
            FillBlock(15, 8, 5, 12, COLOR_WHITE); 
            break;
        case 4:
            clearRectToGradient(3, 4, 23, 17);
            FillBlock(3, 16, 5, 4, COLOR_WHITE);
            FillBlock(9, 12, 5, 8, COLOR_WHITE); 
            FillBlock(15, 8, 5, 12, COLOR_WHITE);
            FillBlock(21, 4, 5, 16, COLOR_WHITE);
            break;
        case 5:
            clearRectToGradient(3, 4, 23, 17);
            DrawRectangle(3, 16, 7, 19, COLOR_WHITE);
            DrawRectangle(9, 12, 13, 19, COLOR_WHITE); 
            DrawRectangle(15, 8, 19, 19, COLOR_WHITE);
            DrawRectangle(21, 4, 25, 19, COLOR_WHITE);
            DrawLine(2, 3, 26, 20, COLOR_RED);
            DrawLine(3, 3, 26, 19, COLOR_RED);
            DrawLine(2, 4, 25, 19, COLOR_RED);
            DrawLine(2, 20, 26, 3, COLOR_RED);
            DrawLine(2, 19, 25, 3, COLOR_RED);
            DrawLine(3, 20, 26, 4, COLOR_RED);
            break;  
        default:
            printf("loraSignal:wrong input\r\n");
            break;
    }
}

static void setName(char *name){
    char user[] = { 0xD3, 0xC3, 0xBB, 0xA7, '\0' }; // "用户"的GBK编码

    clearRectToGradient(101, 67, 128, 32);

    ShowString(26, 35, "ID:", COLOR_WHITE,0x0000, 32,MODE_OVERLAPPING);
    ShowIntNum(73, 35, 2, 1, COLOR_WHITE, 0x0000, 32,MODE_OVERLAPPING);
    ShowChinese(26, 67, user, COLOR_WHITE, 32);
    ShowChar(90, 67, ':', COLOR_WHITE, 0x0000, 32,MODE_OVERLAPPING);
    ShowChinese(106, 67, name, COLOR_WHITE, 32);


}

static void name(bool state){
    char name[] = { 0xB2, 0xDC, 0xB2, 0xD9, '\0' }; // "曹操"的GBK编码

    if(state){
        setName(name);
    }else{
        clear();
    }
}

static void menu(bool state){
    char yuyinmoshi[]     = { 0xD3,0xEF,0xD2,0xF4,0xC4,0xA3,0xCA,0xBD, '\0' };
    char tiaojieliangdu[] = { 0xB5,0xF7,0xBD,0xDA,0xC1,0xC1,0xB6,0xC8, '\0' };
    char xipingshezhi[]   = { 0xCF,0xA2,0xC6,0xC1,0xC9,0xE8,0xD6,0xC3, '\0' };  
    if(state){
        ShowChinese(50,25,yuyinmoshi,COLOR_WHITE,32);
        ShowChinese(50,57,tiaojieliangdu,COLOR_WHITE,32);
        ShowChinese(50,89,xipingshezhi,COLOR_WHITE,32);
    }else{
        clear();
    }
}

static void voiceMenu(bool state){
    char dianantonghua[]  = { 0xB5,0xE3,0xB0,0xB4,0xCD,0xA8,0xBB,0xB0, '\0' };
    char changantonghua[] = { 0xB3,0xA4,0xB0,0xB4,0xCD,0xA8,0xBB,0xB0, '\0' };
    if(state && !isPressTalk){
        ShowChinese(50,25,dianantonghua,COLOR_RED,32);
        ShowChinese(50,57,changantonghua,COLOR_WHITE,32);
    }else if(state && isPressTalk){
        ShowChinese(50,25,dianantonghua,COLOR_WHITE,32);
        ShowChinese(50,57,changantonghua,COLOR_RED,32);
    }else{
        clear();
    }
}

static void chooseMenu(uint8_t state) {
    switch(state){
        case 1:
            clearRectToGradient(17,25,32,96);
            DrawImageBin(17,25,32,32,Image_triangle,COLOR_YELLOW,false);  
            break;   
        case 2:
            clearRectToGradient(17,25,32,96);
            DrawImageBin(17,57,32,32,Image_triangle,COLOR_YELLOW,false);
            break;
        case 3:
            clearRectToGradient(17,25,32,96);
            DrawImageBin(17,89,32,32,Image_triangle,COLOR_YELLOW,false); 
            break;
        default:
            printf("chooseMenu:wrong input");
            break;
    }
    
}

void menuControl(){
    key_scan((void *)&key_state);
    //printf("key_state: %d\n", key_state);
    currentMillis = esp_timer_get_time();

                                                           
    if((key_state != 255) && (isOpen == TRUE) && (key_state != 127) && (key_state != KEY_POWER)){  // 复位首次测量key_state=127（没有点击按键）
        if(isOnLowBatteryWarningPage == TRUE){
            if(isHomepage == TRUE){
                clear();
                name(OPEN);
            }
            else if((isHomepage == FALSE) && (isOnMenu1 == FALSE) && (isOnMenu2 == FALSE) && (isOnMenu3 == FALSE)){
                clear();
                menu(OPEN);
                chooseMenu(menuChooseNum);
            }
            else if(isOnMenu1 == TRUE){
                clear();
                menu1();
            }
            else if(isOnMenu2 == TRUE){
                clear();
                menu2();
            }
            else if(isOnMenu3 == TRUE){
                clear();
                menu3();
                chooseMenu(menuChooseNum);
            }
            isOnLowBatteryWarningPage = FALSE;
            previousMillis = currentMillis;
        }
        else if((key_state == KEY_ENTER) && (isHomepage == TRUE)){
            name(CLOSE);
            menu(OPEN);
            isHomepage = FALSE;
            chooseMenu(1);
            menuChooseNum = 1;
        }
        else if((key_state == KEY_BACK) && (isHomepage == FALSE) && (isOnMenu1 == FALSE) && (isOnMenu2 == FALSE) && (isOnMenu3 == FALSE)){
            menu(CLOSE);
            name(OPEN);
            isHomepage = TRUE;
            isOnMenu1 = FALSE;
            isOnMenu2 = FALSE;
            isOnMenu3 = FALSE;
        }
        else if((key_state == KEY_BACK) && (isHomepage == FALSE) && (isOnMenu1 == TRUE || isOnMenu2 == TRUE || isOnMenu3 == TRUE)){
            clear();
            menu(OPEN);
            isHomepage = FALSE;
            chooseMenu(lastMenuChooseNum);
            menuChooseNum = lastMenuChooseNum;
            isOnMenu1 = FALSE;
            isOnMenu2 = FALSE;
            isOnMenu3 = FALSE;
            is_on_menu3_page2 = FALSE;
        }      
        else if((key_state == KEY_DOWN) && (isHomepage == FALSE) && (isOnMenu1 == FALSE) && (isOnMenu2 == FALSE) && (isOnMenu3 == FALSE)){
            if (menuChooseNum == 1){
                menuChooseNum = 2;
                chooseMenu(2);
            }
            else if (menuChooseNum == 2){
                menuChooseNum = 3;
                chooseMenu(3);
            }  
            else if (menuChooseNum == 3){
                menuChooseNum = 1;
                chooseMenu(1);
            }     
        }
        else if((key_state == KEY_UP) && (isHomepage == FALSE) && (isOnMenu1 == FALSE) && (isOnMenu2 == FALSE) && (isOnMenu3 == FALSE)){
            if (menuChooseNum == 2){
                menuChooseNum = 1;
                chooseMenu(1);
            }
            else if (menuChooseNum == 3){
                menuChooseNum = 2;
                chooseMenu(2);
            }
            else if (menuChooseNum == 1){
                menuChooseNum = 3;
                chooseMenu(3);
            } 
        }
        else if((key_state == KEY_ENTER) && (isHomepage == FALSE) && (isOnMenu1 == FALSE) && (isOnMenu2 == FALSE) && (isOnMenu3 == FALSE)){
            switch(menuChooseNum)
            {
                case 1:
                    clear();
                    isOnMenu1 = TRUE;
                    lastMenuChooseNum = menuChooseNum;
                    menu1();
                    break;
                case 2:
                    clear();
                    isOnMenu2 = TRUE;
                    lastMenuChooseNum = menuChooseNum;
                    menu2();
                    break;
                case 3:
                    clear();
                    isOnMenu3 = TRUE;
                    lastMenuChooseNum = menuChooseNum;
                    if(interval >= SCREEN_TIMEOUT_4){
                        is_on_menu3_page2 = TRUE;
                        menu3();
                        switch (interval)
                        {
                        case SCREEN_TIMEOUT_4:
                            chooseMenu(1);
                            menuChooseNum = 1;
                            break;
                        case SCREEN_TIMEOUT_5:
                            chooseMenu(2);
                            menuChooseNum = 2;
                            break;
                        case SCREEN_TIMEOUT_6:
                            chooseMenu(3);
                            menuChooseNum = 3;
                            break;
                        default:
                            break;
                        }
                    }
                    else{
                        menu3();
                        switch (interval)
                        {
                        case SCREEN_TIMEOUT_1:
                            chooseMenu(1);
                            menuChooseNum = 1;
                            break;
                        case SCREEN_TIMEOUT_2:
                            chooseMenu(2);
                            menuChooseNum = 2;
                            break;
                        case SCREEN_TIMEOUT_3:
                            chooseMenu(3);
                            menuChooseNum = 3;
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                default:
                    printf("menuChooseNum:Error");
                    break;
            }
        }
        else if((key_state == KEY_DOWN) && (isHomepage == FALSE) && ((isOnMenu1 == TRUE) || (isOnMenu2 == TRUE) || (isOnMenu3 == TRUE))){
            if(isOnMenu1 == TRUE){
                if (menuChooseNum == 1){
                    menuChooseNum = 2;
                    chooseMenu(2);
                }
                else if (menuChooseNum == 2){
                    menuChooseNum = 1;
                    chooseMenu(1);
                }  
            }
            else if(isOnMenu2 == TRUE){
                decrease_brightness();
            }
            else if(isOnMenu3 == TRUE){
                if (menuChooseNum == 1){
                    menuChooseNum = 2;
                    chooseMenu(2);
                }
                else if (menuChooseNum == 2){
                    menuChooseNum = 3;
                    chooseMenu(3);
                }  
                else if ((menuChooseNum == 3) && (is_on_menu3_page2 == FALSE)){
                    is_on_menu3_page2 = TRUE;
                    clear();
                    menu3();
                    chooseMenu(1);
                    menuChooseNum = 1;
                }        
            }
        }
        else if((key_state == KEY_UP) && (isHomepage == FALSE) && ((isOnMenu1 == TRUE) || (isOnMenu2 == TRUE) || (isOnMenu3 == TRUE))){
            if(isOnMenu1 == TRUE){
                if (menuChooseNum == 1){
                    menuChooseNum = 2;
                    chooseMenu(2);
                }
                else if (menuChooseNum == 2){
                    menuChooseNum = 1;
                    chooseMenu(1);
                }  
            }
            else if(isOnMenu2 == TRUE){
                increase_brightness();
            }
            else if(isOnMenu3 == TRUE){
                if (menuChooseNum == 2){
                    menuChooseNum = 1;
                    chooseMenu(1);
                }
                else if (menuChooseNum == 3){
                    menuChooseNum = 2;
                    chooseMenu(2);
                }
                else if ((menuChooseNum == 1) && (is_on_menu3_page2 == TRUE)){
                    is_on_menu3_page2 = FALSE;
                    clear();
                    menu3();
                    chooseMenu(3);
                    menuChooseNum = 3;
                } 
            }
        }
        else if((key_state == KEY_ENTER) && (isHomepage == FALSE) && (isOnMenu1 == TRUE)){
            switch(menuChooseNum)
            {
                case 1:
                    if(isPressTalk == TRUE){
                        isPressTalk = FALSE;
                        voiceMenu(TRUE);
                        isTalking = FALSE;
                        if(isTalking == TRUE){
                            TALK_STOP;
                            MIC_ICON_CLOSE;
                            isTalking = FALSE;
                        }
                    }
                    break;
                case 2:
                    if(isPressTalk == FALSE){
                        isPressTalk = TRUE;
                        voiceMenu(TRUE);
                        if(isTalking == TRUE){
                            TALK_STOP;
                            MIC_ICON_CLOSE;
                            isTalking = FALSE;
                        }
                    }
                    break;
                default:
                    printf("menuChooseNum:Error");
                    break;
            }
        }
        else if((key_state == KEY_ENTER) && (isHomepage == FALSE) && (isOnMenu3 == TRUE)){
            if(is_on_menu3_page2 == FALSE){
                switch(menuChooseNum)
                {
                    case 1:
                        if(interval != SCREEN_TIMEOUT_1){
                            interval = SCREEN_TIMEOUT_1;
                            menu3();
                        }
                        break;
                    case 2:
                        if(interval != SCREEN_TIMEOUT_2){
                            interval = SCREEN_TIMEOUT_2;
                            menu3();
                        }
                        break;
                    case 3:
                        if(interval != SCREEN_TIMEOUT_3){
                            interval = SCREEN_TIMEOUT_3;
                            menu3();
                        }
                        break;
                    default:
                        printf("menuChooseNum:Error");
                        break;
                }
            }
            else{
                switch(menuChooseNum)
                {
                    case 1:
                        if(interval != SCREEN_TIMEOUT_4){
                            interval = SCREEN_TIMEOUT_4;
                            menu3();
                        }
                        break;
                    case 2:
                        if(interval != SCREEN_TIMEOUT_5){
                            interval = SCREEN_TIMEOUT_5;
                            menu3();
                        }
                        break;
                    case 3:
                        if(interval != SCREEN_TIMEOUT_6){
                            interval = SCREEN_TIMEOUT_6;
                            menu3();
                        }
                        break;
                    default:
                        printf("menuChooseNum:Error");
                        break;
                }
            }
        }
        else if(key_state == KEY_VOICE){
            if(isPressTalk == TRUE){
                if(isTalking == FALSE){
                    //开始通话
                    TALK_START;
                    MIC_ICON_OPEN;
                    isTalking = TRUE;
                }
            }
            else{
                if(isTalking == FALSE){
                    // 开始通话
                    TALK_START;
                    MIC_ICON_OPEN;
                    isTalking = TRUE; 
                }
                else{
                    // 结束通话
                    TALK_STOP;
                    MIC_ICON_CLOSE;
                    isTalking = FALSE;
                }
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }       
        

        previousMillis = currentMillis;
    }
    else if(key_state == KEY_POWER){
        is_press_power = TRUE;
    }
    else if(((key_state == KEY_UP) || (key_state == KEY_DOWN) || (key_state == KEY_ENTER) || (key_state == KEY_BACK) || (key_state == KEY_VOICE) || (key_state == KEY_SEARCH)) && (isOpen == FALSE)){
        LCD_BACKLIGHT_START;  
        isOpen = TRUE;
        previousMillis = currentMillis;
    }                                    
    else if((key_state == 255) && (isOpen == TRUE) && ((currentMillis - previousMillis) >= interval)){
        LCD_BACKLIGHT_STOP;
        isOpen = FALSE;
    }

    if(isPressTalk == TRUE){
        if((key_state != KEY_VOICE) && (isTalking == TRUE)){
            // 结束通话
            TALK_STOP;
            MIC_ICON_CLOSE;
            isTalking = FALSE;
        }
    }
    else{

    }

    if((is_press_power == TRUE) && (press_power_start == TRUE)){
        press_power_time += currentMillis - last_press_power_time;
        last_press_power_time = currentMillis;
    }
    else if((is_press_power == TRUE) && (press_power_start == FALSE)){
        press_power_start = TRUE;
        last_press_power_time = currentMillis;
    }
    else{
        press_power_time = 0;
    }


    if(key_state != KEY_POWER){
        is_press_power = FALSE;
        press_power_start = FALSE;
    }


    if(press_power_time >= (10*1000*1000)){
        esp_restart();
    }
    else if((press_power_time >= (50*1000)) && (is_press_power == FALSE)){
        if(isOpen == TRUE){
            LCD_BACKLIGHT_STOP;
            isOpen = FALSE;
            press_power_time = 0;
        }
        else{
            LCD_BACKLIGHT_START;  
            isOpen = TRUE;
            previousMillis = currentMillis;
            press_power_time = 0;
        }
    }

    changeBtaIconDisplay(); // 判断是否显示充电图标

    if((currentMillis - previousMillis_readBat) >= interval_readBat){ 
        DisplayBattery();
        previousMillis_readBat = currentMillis;
    }

    
}

static void menu1(){
    voiceMenu(TRUE);
    if(isPressTalk == FALSE){
        chooseMenu(1);
        menuChooseNum = 1;
    }
    else if(isPressTalk == TRUE){
        chooseMenu(2);
        menuChooseNum = 2;
    }
}

static void menu2(){
    if(currentBrightnessLevel == 10){
        DrawFilledCycle(110,33,11,COLOR_WHITE,0x02); 
        FillBlock(111,21,15,13,COLOR_WHITE);
        DrawFilledCycle(126,33,11,COLOR_WHITE,0x01);

        FillBlock(98,34,41,84,COLOR_WHITE);
    }
    else if(currentBrightnessLevel < 10){
        DrawFilledCycle(110,33,11,COLOR_DARKGRAY,0x02); 
        FillBlock(111,21,15,13,COLOR_DARKGRAY);
        DrawFilledCycle(126,33,11,COLOR_DARKGRAY,0x01);
        FillBlock(98,34,41,3,COLOR_DARKGRAY);

        FillBlock(98,37,41,(9 * (9-currentBrightnessLevel)),COLOR_DARKGRAY);
        FillBlock(98,(37 + (9 * (9-currentBrightnessLevel))),41,(9 * currentBrightnessLevel),COLOR_WHITE);
    }

    DrawFilledCycle(110,118,11,COLOR_WHITE,0x04);
    FillBlock(111,118,15,13,COLOR_WHITE);
    DrawFilledCycle(126,118,11,COLOR_WHITE,0x08);

    DrawImageBin(57,21,32,32,Image_brightness,0xFE60,FALSE);
}


static void menu3(){
    char miaozhong[]  = { 0xC3,0xEB,0xD6,0xD3, '\0' };
    char fenzhong[]   = { 0xB7,0xD6,0xD6,0xD3, '\0' };
    char yongbu[]     = { 0xD3,0xC0,0xB2,0xBB, '\0' };

    if(is_on_menu3_page2 == FALSE){
        switch (interval) {
        case SCREEN_TIMEOUT_1:
            ShowIntNum(50,25,30,2,COLOR_RED,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(82,25,miaozhong,COLOR_RED,32);
            ShowIntNum(50,57,1,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_WHITE,32);
            ShowIntNum(50,89,2,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,89,fenzhong,COLOR_WHITE,32);
            break;
        case SCREEN_TIMEOUT_2:
            ShowIntNum(50,25,30,2,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(82,25,miaozhong,COLOR_WHITE,32);
            ShowIntNum(50,57,1,1,COLOR_RED,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_RED,32);
            ShowIntNum(50,89,2,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,89,fenzhong,COLOR_WHITE,32);
            break;
        case SCREEN_TIMEOUT_3:
            ShowIntNum(50,25,30,2,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(82,25,miaozhong,COLOR_WHITE,32);
            ShowIntNum(50,57,1,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_WHITE,32);
            ShowIntNum(50,89,2,1,COLOR_RED,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,89,fenzhong,COLOR_RED,32);
            break;
        default:
            ShowIntNum(50,25,30,2,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(82,25,miaozhong,COLOR_WHITE,32);
            ShowIntNum(50,57,1,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_WHITE,32);
            ShowIntNum(50,89,2,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,89,fenzhong,COLOR_WHITE,32);
            break;
        }
    }
    else{
        switch (interval) {
        case SCREEN_TIMEOUT_4:
            ShowIntNum(50,25,3,1,COLOR_RED,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,25,fenzhong,COLOR_RED,32);
            ShowIntNum(50,57,5,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_WHITE,32);
            ShowChinese(50,89,yongbu,COLOR_WHITE,32);
            break;
        case SCREEN_TIMEOUT_5:
            ShowIntNum(50,25,3,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,25,fenzhong,COLOR_WHITE,32);
            ShowIntNum(50,57,5,1,COLOR_RED,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_RED,32);
            ShowChinese(50,89,yongbu,COLOR_WHITE,32);
            break;
        case SCREEN_TIMEOUT_6:
            ShowIntNum(50,25,3,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,25,fenzhong,COLOR_WHITE,32);
            ShowIntNum(50,57,5,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_WHITE,32);
            ShowChinese(50,89,yongbu,COLOR_RED,32);
            break;
        default:
            ShowIntNum(50,25,3,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,25,fenzhong,COLOR_WHITE,32);
            ShowIntNum(50,57,5,1,COLOR_WHITE,0x0000,32,MODE_OVERLAPPING);
            ShowChinese(66,57,fenzhong,COLOR_WHITE,32);
            ShowChinese(50,89,yongbu,COLOR_WHITE,32);
            break;
        }
    }
}
// 初始化LED PWM
static void pwm_init()
{
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL,
        .duty       = 0,  // 初始亮度为 0
        .gpio_num   = LEDC_OUTPUT_IO,
        .speed_mode = LEDC_MODE,
        .timer_sel  = LEDC_TIMER
    };
    ledc_channel_config(&ledc_channel);
}

// 设置PWM亮度（输入0-255）
static void set_backlight_brightness(uint8_t brightness)
{
    uint32_t duty = (255 - brightness);  // 低电平越多，亮度越高
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// 切换到手动控制
static void closeBacklight(bool enable)
{
    if (enable)
    {
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);  // 释放PWM
        // 重新初始化 GPIO 为普通输出模式
        gpio_reset_pin(LEDC_OUTPUT_IO);  // 先复位，避免冲突
        gpio_set_direction(LEDC_OUTPUT_IO, GPIO_MODE_OUTPUT);
        gpio_set_level(LEDC_OUTPUT_IO, 1);
    }
    else
    {
        pwm_init();  // 重新初始化PWM
        set_backlight_brightness(brightness_values[currentBrightnessLevel]);
    }
}

static void increase_brightness()
{
    if (currentBrightnessLevel < BRIGHTNESS_LEVELS - 1) {
        if(currentBrightnessLevel == 9){
            DrawFilledCycle(110,33,11,COLOR_WHITE,0x02); 
            FillBlock(111,21,15,13,COLOR_WHITE);
            DrawFilledCycle(126,33,11,COLOR_WHITE,0x01);
            FillBlock(98,34,41,3,COLOR_WHITE);
        }
        else{
            FillBlock(98,(37+9*(8-currentBrightnessLevel)),41,9,COLOR_WHITE);
        }
        currentBrightnessLevel++;
        set_backlight_brightness(brightness_values[currentBrightnessLevel]);
    }
}

static void decrease_brightness()
{
    if (currentBrightnessLevel > 0) {
        if(currentBrightnessLevel == 10){
            DrawFilledCycle(110,33,11,COLOR_DARKGRAY,0x02); 
            FillBlock(111,21,15,13,COLOR_DARKGRAY);
            DrawFilledCycle(126,33,11,COLOR_DARKGRAY,0x01);
            FillBlock(98,34,41,3,COLOR_DARKGRAY);
        }
        else{
            FillBlock(98,(37+9*(9-currentBrightnessLevel)),41,9,COLOR_DARKGRAY);
        }
        currentBrightnessLevel--;
        set_backlight_brightness(brightness_values[currentBrightnessLevel]);
    }
}

void test(){
    vTaskDelay(pdMS_TO_TICKS(2000));

    pwm_init();
    set_backlight_brightness(255);

    vTaskDelay(pdMS_TO_TICKS(2000));

    closeBacklight(TRUE);
}