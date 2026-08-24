#pragma once

/* 常用颜色 */
#define WHITE           0xFFFFFF      /* 白色 */
#define BLACK           0x000000      /* 黑色 */
#define RED             0xFF0000      /* 红色 */
#define GREEN           0x00FF00      /* 绿色 */
#define BLUE            0x0000FF      /* 蓝色 */ 
#define MAGENTA         0xFF00FF      /* 品红色/紫红色 = BLUE + RED */
#define YELLOW          0xFFFF00      /* 黄色 = GREEN + RED */
#define CYAN            0x00FFFF      /* 青色 = GREEN + BLUE */  

// 测试参数联合体：兼容所有LCD测试函数的入参
typedef union {
    struct {
        int bg_color;
        int border_color;
        int border_width;
    } border;

    struct {
        int start_color;
        int end_color;
        int is_gradient;
    } color;

    // 图片路径
    const char *img_path;
    int gray_level_num;
} lcd_test_param_t;

/* LCD测试项结构体 */
typedef struct {
    const char *name;               //测试项名称
    void (*func)(lcd_test_param_t param);      //测试项函数指针
    lcd_test_param_t param;         //函数参数
    uint32_t delay_ms;              //测试项执行后延时时间
    bool enabled;                   //是否启用
}lcd_test_item_t;

#ifdef AIC_USING_HT68_DEMO_A01
#define KEYADC_TEST_CHANNLE     5
#else 
#define KEYADC_TEST_CHANNLE     7
#endif 

typedef enum KEY_FLAG {
    KEY_NONE = 0,
    KEY_UP = 1,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
}key_flag_t;

#define LCD_EVENT_MODE      (1u << 1)   // UP：切 自动/手动
#define LCD_EVENT_NEXT      (1u << 2)   // DOWN：下一画面
#define LCD_EVENT_PREV      (1u << 3)   // 上一画面（可选）
#define LCD_EVENT_BRIGHT    (1u << 4)   // 上一画面（可选）

static void lcd_decode_img_path(char *file_path);

void lcd_test_start(void);

int lcd_ge_begin(void);
