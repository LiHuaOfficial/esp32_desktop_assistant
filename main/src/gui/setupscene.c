#include "setupscene.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"

#include "lvgl.h"

#include "myWifi.h"
#include "inputscene.h"
#include "status_routine.h"

#define MENU_BTN_NUM 8



extern SemaphoreHandle_t xGuiSemaphore;
extern EventGroupHandle_t eventGroup_lvgl2espWifi;

extern lv_obj_t* mainScene;
extern lv_obj_t* setupScene;

extern lv_indev_t * indev_keypad;

// extern lv_group_t* group_mainScene;
// extern lv_group_t* group_setupScene_default;
// lv_group_t* group_setupScene_inSubMenu;

static lv_style_t style_menuObj;//这个样式将用于所有的菜单选项
static lv_style_t style_menuBtn;

static lv_obj_t* currentSubMenu=NULL;

const char* text_btnMenu[MENU_BTN_NUM] = { "Return to main","menu1","menu2","Wifi","menu4"};

static void Btn_ShowMenu_Handler(lv_event_t *e);
static void BtnMenu_Handler(lv_event_t* e);

static void AnimMenu_Handler(void* var, int32_t v);

//widget包括label和固定类型

static lv_obj_t* SubMenu_Menu1_Create();
static lv_obj_t* SubMenu_Menu2_Create();
static lv_obj_t* SubMenu_WifiMenu_Create();

void Task_SetupScene(void* arg){

    xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
    
    //输入组初始化
    // group_setupScene_default=lv_group_create();
    // group_setupScene_inSubMenu=lv_group_create();
    //初始化样式
    lv_obj_set_style_bg_color(setupScene,lv_color_darken(lv_color_hex(0xffffff),LV_OPA_10),0);

    //这个样式将用于所有的菜单选项
    lv_style_init(&style_menuObj);
    lv_style_set_bg_color(&style_menuObj, lv_color_hex(0xffffff));
    lv_style_set_text_color(&style_menuObj, lv_color_hex(0));

    lv_style_set_border_width(&style_menuObj, 0);
    lv_style_set_shadow_width(&style_menuObj, 0);//默认样式自带一点点阴影
    lv_style_set_radius(&style_menuObj, 4);
    
    lv_style_init(&style_menuBtn);
    lv_style_set_bg_color(&style_menuBtn, lv_color_hex(0xffffff));
    lv_style_set_text_color(&style_menuBtn, lv_color_hex(0));

    lv_style_set_border_width(&style_menuBtn, 0);
    lv_style_set_shadow_width(&style_menuBtn, 10);//默认样式自带一点点阴影
    lv_style_set_radius(&style_menuBtn, 4);
    
    lv_style_set_width(&style_menuBtn,lv_disp_get_hor_res(NULL) / 2-10);
    lv_style_set_height(&style_menuBtn,lv_disp_get_ver_res(NULL)/8);
    
    //初始化控件
    //显示主菜单的按钮
    lv_obj_t* btn_showMenu = lv_btn_create(setupScene);
    lv_obj_t* label_btn_showMenu = lv_label_create(btn_showMenu);
    lv_label_set_text(label_btn_showMenu, "Menu");
    lv_obj_add_flag(btn_showMenu,LV_OBJ_FLAG_CHECKABLE);
    
    lv_obj_update_layout(btn_showMenu);
    //菜单框
    lv_obj_t* widget_menu = lv_obj_create(setupScene);
    lv_obj_set_size(widget_menu, lv_disp_get_hor_res(NULL) / 10*7, lv_disp_get_ver_res(NULL));
    lv_obj_set_style_bg_opa(widget_menu, LV_OPA_50, 0);
    lv_obj_set_style_radius(widget_menu, 0, 0);
    lv_obj_set_flex_flow(widget_menu, LV_FLEX_FLOW_COLUMN);
    

    lv_obj_add_flag(widget_menu, LV_OBJ_FLAG_HIDDEN);

    lv_obj_update_layout(widget_menu);
    //框内功能按钮
    

    for (uint16_t i = 0; i < MENU_BTN_NUM; i++) {
        lv_obj_t* btn_menu = lv_btn_create(widget_menu);
        lv_obj_t* label_btnMenu = lv_label_create(btn_menu);

        lv_obj_set_size(btn_menu,lv_obj_get_width(widget_menu)-30,lv_obj_get_height(widget_menu)/4-10);
        lv_label_set_text(label_btnMenu,text_btnMenu[i]);
        lv_obj_set_align(label_btnMenu, LV_ALIGN_LEFT_MID);

        lv_obj_add_style(btn_menu, &style_menuBtn, LV_PART_MAIN);
        //lv_obj_set_style_border_width(btn_menu, 10,0);

        lv_obj_add_event_cb(btn_menu, BtnMenu_Handler, LV_EVENT_CLICKED, (void*)i);//转换为指针传值

        
    }

    lv_obj_move_foreground(btn_showMenu);

    lv_obj_add_event_cb(btn_showMenu,Btn_ShowMenu_Handler,LV_EVENT_CLICKED,widget_menu);

    lv_obj_add_style(btn_showMenu, &style_menuObj, LV_PART_MAIN);
    
    currentSubMenu=SubMenu_Menu1_Create();//创建默认菜单
    xSemaphoreGive(xGuiSemaphore);

    printf("inSetupScene:%lu\n",uxTaskGetStackHighWaterMark2(xTaskGetCurrentTaskHandle()));
    vTaskDelete(xTaskGetCurrentTaskHandle());
}

//这个函数用于实现通过左上按钮显示&隐藏菜单栏的功能
void Btn_ShowMenu_Handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn_showMenu = (lv_obj_t* )lv_event_get_target(e);
    lv_obj_t* widget_menu = (lv_obj_t*)lv_event_get_user_data(e);

    lv_obj_t* label_btn_showMenu = lv_obj_get_child(btn_showMenu, 0);

    if (code == LV_EVENT_CLICKED) {
        lv_anim_t anim_menu;
        lv_anim_init(&anim_menu);
        lv_anim_set_var(&anim_menu, widget_menu);
        lv_anim_set_time(&anim_menu, 500);
        lv_anim_set_exec_cb(&anim_menu,AnimMenu_Handler);
        lv_anim_set_path_cb(&anim_menu,lv_anim_path_ease_in_out);

        if (lv_obj_get_state(btn_showMenu) & LV_STATE_CHECKED) {//如此确定状态
            //按下显示菜单

            lv_obj_clear_flag(widget_menu, LV_OBJ_FLAG_HIDDEN);
            /*简单的动画实现*/
            //lv_obj_fade_in(widget_menu, 500, 100);

            /*使用anim的动画实现*/
            lv_anim_set_values(&anim_menu, -lv_obj_get_width(widget_menu), 0);
            lv_anim_start(&anim_menu);
            lv_label_set_text(label_btn_showMenu, "Close");

            // lv_indev_set_group(indev_keypad,group_setupScene_default);
        }
        else {
            //按下菜单消失

            /*简单的动画实现*/
            //lv_obj_fade_out(widget_menu, 500, 100);

            /*使用anim的动画实现*/
            lv_anim_set_values(&anim_menu, 0, -lv_obj_get_width(widget_menu));
            lv_anim_start(&anim_menu);
            lv_label_set_text(label_btn_showMenu,"Menu");
            //lv_obj_add_flag(widget_menu, LV_OBJ_FLAG_HIDDEN);
            
            // lv_indev_set_group(indev_keypad,group_setupScene_inSubMenu);
        }       
    }
}

//这个函数实现菜单栏中按钮的具体功能
void BtnMenu_Handler(lv_event_t* e)
{
    uint16_t btnIndex = (uint16_t)lv_event_get_user_data(e);

    printf("current btn index:%u\n", btnIndex);

    /*暂时采用每次选择
    都重新创建组件的方式
    保留父组件指针下次选择删除*/
    //下面这个变量现在改成全局的了
    //static lv_obj_t* currentSubMenu=NULL;
    
    if (currentSubMenu) {//再次选择的时候，清除之前内容
        lv_obj_del(currentSubMenu);
        currentSubMenu = NULL;
    }

    switch (btnIndex)
    {
    case 0:
        //返回主界面
        lv_scr_load_anim(mainScene,LV_SCR_LOAD_ANIM_FADE_ON,500,100,false);
        // lv_indev_set_group(indev_keypad,group_mainScene);
        break;
    case 1:
        currentSubMenu=SubMenu_Menu1_Create();
        break;
    case 2:
        currentSubMenu = SubMenu_Menu2_Create();
        break;
    case 3:
        currentSubMenu =SubMenu_WifiMenu_Create();
        break;
    default:
        if (currentSubMenu) {
            //lv_group_remove_obj(currentSubMenu);
            lv_obj_del(currentSubMenu);
            currentSubMenu = NULL;
        }
        printf("no implements\n");
        break;
    }
    //如果创建了新界面
    //if(currentSubMenu) lv_group_add_obj(group_setupScene_inSubMenu,currentSubMenu);
}

//菜单栏的移动的回调函数
void AnimMenu_Handler(void* var, int32_t v)
{
    lv_obj_set_x((lv_obj_t*)var, v);
}
 
lv_obj_t* SubMenu_Stuff_Create(lv_obj_t* parent,const char* text,const char* objText,
                                submenu_type_t type, lv_event_cb_t stuff_event_cb, uint32_t value,
                                void* args)
{
    lv_obj_t* widget = lv_obj_create(parent);//框
    lv_obj_set_size(widget,lv_disp_get_hor_res(NULL)-20,lv_disp_get_ver_res(NULL)/4);
    lv_obj_update_layout(widget);
    
    lv_obj_t* label = lv_label_create(widget);//文字说明
    lv_label_set_text(label,text);

    lv_obj_t* stuff = NULL;
    switch (type)
    {
    case SUBMENU_TYPE_SLIDER:
        stuff = lv_slider_create(widget);
        lv_obj_set_width(stuff,lv_obj_get_width(widget)/2);
        if (stuff_event_cb)
            lv_obj_add_event_cb(stuff, stuff_event_cb, LV_EVENT_ALL, args);
        break;
    case SUBMENU_TYPE_CHECKBOX:
        stuff = lv_checkbox_create(widget);
        if (stuff_event_cb) 
            lv_obj_add_event_cb(stuff, stuff_event_cb, LV_EVENT_ALL, args);
        break;
    case SUBMENU_TYPE_SWITCH:
        stuff = lv_switch_create(widget);

        if(value) lv_obj_add_state(stuff,LV_STATE_CHECKED);
        else lv_obj_clear_state(stuff,LV_STATE_CHECKED);

        if (stuff_event_cb) 
            lv_obj_add_event_cb(stuff, stuff_event_cb, LV_EVENT_ALL, args);
        break;
    case SUBMENU_TYPE_BUTTON:
        stuff=lv_btn_create(widget);

        lv_obj_t* tmpLabel=lv_label_create(stuff);
        if(objText) lv_label_set_text(tmpLabel,objText);

        lv_obj_add_style(stuff,&style_menuObj,LV_PART_MAIN);
        lv_obj_set_style_border_width(stuff,1,LV_PART_MAIN);

        if (stuff_event_cb) 
            lv_obj_add_event_cb(stuff, stuff_event_cb, LV_EVENT_ALL, args);
        break;
    case SUBMENU_TYPE_LABEL:
        stuff=lv_label_create(widget);
        if(objText) lv_label_set_text(stuff,objText);
        
        if(strcmp(text,LV_SYMBOL_WIFI)==0){
            lv_obj_add_flag(widget,LV_OBJ_FLAG_CLICKABLE);//在wifi中可以选中
            lv_obj_add_event_cb(widget,stuff_event_cb,LV_EVENT_ALL,NULL);
        } 
        break;
    default:
        break;
    }

    //aligning job
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    if (stuff) lv_obj_align(stuff, LV_ALIGN_BOTTOM_RIGHT,0,0);

    lv_obj_add_style(widget,&style_menuObj,LV_PART_MAIN);
    return widget;
}

/*具体菜单的创建*/

lv_obj_t* SubMenu_Menu1_Create()
{
    lv_obj_t* subMenu=lv_obj_create(setupScene);
    lv_obj_set_style_bg_color(subMenu, lv_color_darken(lv_color_hex(0xffffff), LV_OPA_10), 0);
    lv_obj_set_style_border_width(subMenu, 0, 0);
    lv_obj_set_size(subMenu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_y(subMenu, 30);
    lv_obj_move_background(subMenu);

    lv_obj_set_flex_flow(subMenu, LV_FLEX_FLOW_COLUMN);

    SubMenu_Stuff_Create(subMenu, "A slider",NULL, SUBMENU_TYPE_SLIDER, NULL, 0,NULL);
    SubMenu_Stuff_Create(subMenu, "A switch",NULL, SUBMENU_TYPE_SWITCH, NULL, 0,NULL);


    return subMenu;
}

lv_obj_t* SubMenu_Menu2_Create()
{
    lv_obj_t* subMenu = lv_obj_create(setupScene);
    lv_obj_set_style_bg_color(subMenu, lv_color_darken(lv_color_hex(0xffffff), LV_OPA_10), 0);
    lv_obj_set_style_border_width(subMenu, 0, 0);
    lv_obj_set_size(subMenu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_y(subMenu, 30);
    lv_obj_move_background(subMenu);

    lv_obj_set_flex_flow(subMenu, LV_FLEX_FLOW_COLUMN);

    SubMenu_Stuff_Create(subMenu, "A checkbox",NULL,SUBMENU_TYPE_CHECKBOX, NULL, 0,NULL);

    return subMenu;
}

static void WifiMenu_Refresh_Switch_Handler(lv_event_t* e){
    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* Switch=lv_event_get_current_target(e);
    lv_obj_t* subMenu=lv_event_get_user_data(e);

    if(code==LV_EVENT_VALUE_CHANGED && lv_obj_check_type(Switch,&lv_switch_class)){       
        if(lv_obj_has_state(Switch,LV_STATE_CHECKED)){
            //直接开始扫描
            esp_wifi_start();//?
            common_status.menu_wifi_switch=true;
            xTaskCreate(Task_WifiScan,"WifiScan",4096,subMenu,2,NULL);
        }else{
            common_status.menu_wifi_switch=false;
            esp_wifi_stop();
        }
    }else if(code==LV_EVENT_CLICKED && lv_obj_check_type(Switch,&lv_btn_class)){
        //处理刷新按钮
        if(common_status.menu_wifi_switch==true){
            xTaskCreate(Task_WifiScan,"WifiScan",4096,subMenu,2,NULL);
        }
    }
}

lv_obj_t* SubMenu_WifiMenu_Create()
{
    lv_obj_t* subMenu = lv_obj_create(setupScene);
    lv_obj_set_style_bg_color(subMenu, lv_color_darken(lv_color_hex(0xffffff), LV_OPA_10), 0);
    lv_obj_set_style_border_width(subMenu, 0, 0);
    lv_obj_set_size(subMenu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_y(subMenu, 30);
    lv_obj_move_background(subMenu);
    lv_obj_set_flex_flow(subMenu, LV_FLEX_FLOW_COLUMN);

    SubMenu_Stuff_Create(subMenu,"Wifi",NULL,SUBMENU_TYPE_SWITCH, WifiMenu_Refresh_Switch_Handler,
                        common_status.menu_wifi_switch,subMenu);
    SubMenu_Stuff_Create(subMenu,"Refresh",LV_SYMBOL_REFRESH,SUBMENU_TYPE_BUTTON,WifiMenu_Refresh_Switch_Handler,0,subMenu);
    return subMenu;
}

void WifiMenu_Connect_Label_Handler(lv_event_t* e){
    /*
    点击特定wifi的回调函数
    一个处理输入的界面将被创建

    */
    lv_event_code_t code=lv_event_get_code(e);
    
    lv_obj_t* ssidLabel=lv_obj_get_child(lv_event_get_target(e),1);
    
    if(code==LV_EVENT_CLICKED){
        //显示inputScene
        InputScene_Show(INPUT_CB_WIFI_PWD,ssidLabel);
    }
}