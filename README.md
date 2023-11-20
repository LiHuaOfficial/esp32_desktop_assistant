# esp32_desktop_assistant
esp32配合lvgl实现的简单桌面助手

需要以下组件
```
idf.py add-dependency "lvgl/lvgl^8.2.0"

idf.py add-dependency "vitoralho/lvgl_esp32_drivers^1.0.5"
```

## 修改lvgl_esp32_driver
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