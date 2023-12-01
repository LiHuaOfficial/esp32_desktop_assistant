# ESP32桌面助手
esp32配合lvgl实现的简单桌面助手

目前实现的功能
+ 主页面（未添加内容）  
+ 设置界面
    + 连接Wifi
+ 全局的消息提示
+ 从心知天气获取JSON解析并显示

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

## 可能出现的问题
+ ### 找不到头文件
将.vscode\c_cpp_properties.json中的"compilerPath"更改为ESP-IDF build project时显示的编译器路径即可
```
{
    "configurations": [
        {
            "name": "ESP-IDF",
            "compilerPath": "D:/Applications/ESP32/.espressif/tools/xtensa-esp32-elf/esp-12.2.0_20230208/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc.exe",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "includePath": [
                "${config:idf.espIdfPath}/components/**",
                "${config:idf.espIdfPathWin}/components/**",
                "${config:idf.espAdfPath}/components/**",
                "${config:idf.espAdfPathWin}/components/**",
                "${workspaceFolder}/**"
            ],
            "browse": {
                "path": [
                    "${config:idf.espIdfPath}/components",
                    "${config:idf.espIdfPathWin}/components",
                    "${config:idf.espAdfPath}/components/**",
                    "${config:idf.espAdfPathWin}/components/**",
                    "${workspaceFolder}"
                ],
                "limitSymbolsToIncludedHeaders": false
            }  
        }
    ],
    "version": 4
}

```