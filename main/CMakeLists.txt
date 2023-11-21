# SET(PROJECT_SOURCE_DIR D:/codes/ESP32/esp32_desktop_assistant) esp-idf默认项目更目录是下面这个
# SET(c_srcs)

file(GLOB_RECURSE c_srcs 
    "${PROJECT_DIR}/main/*.c"
    "${PROJECT_DIR}/main/scr/*.c")

MESSAGE(STATUS "in c_srcs " ${c_srcs})

idf_component_register(SRCS ${c_srcs}#应当以变量形式使用！
                    INCLUDE_DIRS "." "inc")

#-----------------------------------------------------------------#
# idf_component_register(SRCS "main.c" "src/keypad.c" "src/mygui.c"
#                     INCLUDE_DIRS "." "inc")
