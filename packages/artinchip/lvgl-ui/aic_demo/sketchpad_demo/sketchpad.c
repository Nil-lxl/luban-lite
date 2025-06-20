#include "sketchpad.h"


#define CUR_HOR_RES 720
#define CUR_VER_RES 720

static lv_obj_t* scr;
static lv_obj_t* canvas;
static lv_layer_t layer;
static lv_draw_line_dsc_t line_dsc;
static lv_point_t prev_p;
static lv_timer_t* draw_timer;

void sketchpad_cb(lv_event_t* e) {
	lv_event_code_t code=lv_event_get_code(e);

	if (code==LV_EVENT_PRESSED||code==LV_EVENT_PRESSING) {
		lv_indev_t* indev=lv_indev_get_act();
		lv_point_t current_p;
        lv_canvas_init_layer(canvas,&layer);
		lv_indev_get_point(indev,&current_p);
		if (prev_p.x!=0||prev_p.y!=0) {
			line_dsc.p1.x=prev_p.x;
			line_dsc.p1.y=prev_p.y;
			line_dsc.p2.x=current_p.x;
			line_dsc.p2.y=current_p.y;
		}
		lv_draw_line(&layer,&line_dsc);
		lv_canvas_finish_layer(canvas,&layer);

		rt_kprintf("P1:%d,%d  P2:%d,%d\n",prev_p.x,prev_p.y,current_p.x,current_p.y);
		prev_p=current_p;

	}else if (code==LV_EVENT_RELEASED) {
		prev_p.x=0;
		prev_p.y=0;
	}

}
void draw_timer_cb(lv_timer_t* timer) {
	line_dsc.p1.x++;
	line_dsc.p1.y++;
	line_dsc.p2.x++;
	line_dsc.p2.y++;
	lv_draw_line(&layer,&line_dsc);
	lv_canvas_finish_layer(canvas,&layer);
}
void sketchpad_init(void){
    scr=lv_obj_create(lv_scr_act());
    lv_obj_set_size(scr,CUR_HOR_RES,CUR_VER_RES);
	lv_obj_set_style_radius(scr,0,0);
	lv_obj_set_style_pad_all(scr,0,0);
	// lv_obj_clear_flag(scr,LV_OBJ_FLAG_SCROLLABLE);

	static lv_color_t canvas_buffer[CUR_HOR_RES * CUR_VER_RES ];
	canvas=lv_canvas_create(scr);
	// lv_obj_set_style_border_width(canvas,0,0);
	lv_canvas_set_buffer(canvas,canvas_buffer,CUR_HOR_RES,CUR_VER_RES,LV_COLOR_FORMAT_RGB888);
	lv_canvas_fill_bg(canvas,lv_palette_lighten(LV_PALETTE_BLUE,3),0);

	lv_canvas_init_layer(canvas,&layer);

	lv_draw_line_dsc_init(&line_dsc);
	line_dsc.color=lv_palette_main(LV_PALETTE_RED);
	line_dsc.width=5;
	line_dsc.round_start=1;
	line_dsc.round_end=1;

    // line_dsc.p1.x=20;
	// line_dsc.p1.y=20;
	// line_dsc.p2.x=200;
	// line_dsc.p2.y=200;
    // lv_draw_line(&layer,&line_dsc);
	// lv_canvas_finish_layer(canvas,&layer);

    lv_obj_add_flag(canvas,LV_OBJ_FLAG_CLICKABLE);
	// lv_obj_set_scrollbar_mode(scr,LV_SCROLLBAR_MODE_OFF);
    // lv_timer_create(draw_timer_cb,100,NULL);

	lv_obj_add_event_cb(canvas,sketchpad_cb,LV_EVENT_ALL,NULL);
    
}
void ui_init(void){
    sketchpad_init();
}