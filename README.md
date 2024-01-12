# ESP32桌面助手
esp32配合lvgl实现的简单桌面助手 

基于esp-idf v5.1.1 & LVGL v8.2.1

目前实现的功能
+ 按键输入（上下左右&确定）
+ 主页面
    + 显示天气（从心知天气获取JSON并解析）
    + 显示时间&日期（通过NTP服务器对时）
    + 基于SHT30显示室内温湿度
    + 小豆泥的GIF
+ 设置界面
    + 连接Wifi
    + 更改当前位置（用来获取天气）
+ 全局的消息提示


## 如何使用

+ ### 硬件接线
    + 显示屏（menuconfig中定义）
        + MOSI 23
        + CLK 18
        + CS 5
        + DC 27
        + RESET 33
        + BK 32
    + 按钮（input_device.h中的宏定义）
        + UP 15
        + DOWN 4
        + LEFT 19
        + RIGHT 21
        + MID 22
    + SHT3x
        + SCL 25
        + SDA 26
+ ### 添加组件
需要添加以下组件
```
idf.py add-dependency "lvgl/lvgl^8.2.0"

idf.py add-dependency "vitoralho/lvgl_esp32_drivers^1.0.5"
```

+ ### 修改lvgl_esp32_driver
在menuconfig中配置好屏幕设置后  
在对应屏幕的.c文件中修改旧版本GPIO初始化方式，如下
```
gpio_pad_select_gpio(XXXX_DC);
	
gpio_set_direction(XXXX_DC,GPIO_MODE_OUTPUT);
```
为新版本初始化方式
```
gpio_reset_pin(XXXX_DC);
    
gpio_set_direction(XXXX_DC, GPIO_MODE_OUTPUT);
```
同样在esp_lcd_backlight.c中修改GPIO的初始化，如果不使用pwm控制的话将背光IO直接置高电平即可

+ ### 修改IDF.PY menuconfig
    + **LVGL** 将其中的LVGL configuration中color项内的**Images pixels with this color will not be drawn**项改为0x0000，使得图片不显示黑边        
    （生成时颜色格式为CF_TRUE_COLOR_CHROMA，LVGL将不会渲染为0x0000的像素）
    + **LVGL** 启用内置字体 **Montserrat14、30、48**
    + **ESP Components** 打开ESP-TLS中的**Skip server certification**
    + 启用GIF_DECODER
    + 将LVGL分配的内存调至64kb

+ ### 首次下载
    + 取消根目录下spiffs_create_partition_image(storage resources FLASH_IN_PROJECT)的注释
## 可能出现的问题
+ ### 找不到头文件
使用idf提供的工具自动添加路径
```
view->command palette->ESP-IDF:add vscode configration folder
```