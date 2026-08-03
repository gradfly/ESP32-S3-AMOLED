/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
  
	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_carousel_1;
	lv_obj_t *screen_carousel_1_element_1;
	lv_obj_t *screen_carousel_1_element_2;
	lv_obj_t *screen_carousel_1_element_3;
	lv_obj_t *screen_carousel_1_element_4;
	lv_obj_t *screen_cont_4;
	lv_obj_t *screen_img_3;
	lv_obj_t *screen_img_1;
	lv_obj_t *screen_img_2;
	lv_obj_t *screen_cont_5;
	lv_obj_t *screen_img_4;
	lv_obj_t *screen_img_5;
	lv_obj_t *screen_img_6;
	lv_obj_t *screen_label_1;
	lv_obj_t *screen_label_2;
	lv_obj_t *screen_label_3;
	lv_obj_t *screen_label_4;
	lv_obj_t *screen_label_5;
	lv_obj_t *screen_label_7;
	lv_obj_t *screen_label_8;
	lv_obj_t *screen_label_6;
	lv_obj_t *screen_spangroup_1;
	lv_span_t *screen_spangroup_1_span;
	lv_obj_t *screen_cont_2;
	lv_obj_t *screen_label_15;
	lv_obj_t *screen_imgbtn_1;
	lv_obj_t *screen_imgbtn_1_label;
	lv_obj_t *screen_imgbtn_2;
	lv_obj_t *screen_imgbtn_2_label;
	lv_obj_t *screen_cont_1;
	lv_obj_t *screen_list_1;
	lv_obj_t *screen_list_1_item0;
	lv_obj_t *screen_list_1_item1;
	lv_obj_t *screen_list_1_item2;
	lv_obj_t *screen_list_1_item3;
	lv_obj_t *screen_list_1_item4;
	lv_obj_t *screen_list_1_item5;
	lv_obj_t *screen_list_1_item6;
	lv_obj_t *screen_list_1_item7;
	lv_obj_t *screen_list_1_item8;
	lv_obj_t *screen_list_1_item9;
	lv_obj_t *screen_list_1_item10;
	lv_obj_t *screen_list_1_item11;
	lv_obj_t *screen_list_1_item12;
	lv_obj_t *screen_list_1_item13;
	lv_obj_t *screen_list_1_item14;
	lv_obj_t *screen_list_1_item15;
	lv_obj_t *screen_list_1_item16;
	lv_obj_t *screen_list_1_item17;
	lv_obj_t *screen_list_1_item18;
	lv_obj_t *screen_list_1_item19;
	lv_obj_t *screen_list_1_item20;
	lv_obj_t *screen_label_13;
	lv_obj_t *screen_btn_1;
	lv_obj_t *screen_btn_1_label;
	lv_obj_t *screen_btn_4;
	lv_obj_t *screen_btn_4_label;
	lv_obj_t *screen_cont_3;
	lv_obj_t *screen_btn_2;
	lv_obj_t *screen_btn_2_label;
	lv_obj_t *screen_label_14;
	lv_obj_t *screen_list_2;
	lv_obj_t *screen_list_2_item0;
	lv_obj_t *screen_list_2_item1;
	lv_obj_t *screen_list_2_item2;
	lv_obj_t *screen_list_2_item3;
	lv_obj_t *screen_list_2_item4;
	lv_obj_t *screen_list_2_item5;
	lv_obj_t *screen_list_2_item6;
	lv_obj_t *screen_list_2_item7;
	lv_obj_t *screen_list_2_item8;
	lv_obj_t *screen_list_2_item9;
	lv_obj_t *screen_list_2_item10;
	lv_obj_t *screen_list_2_item11;
	lv_obj_t *screen_list_2_item12;
	lv_obj_t *screen_list_2_item13;
	lv_obj_t *screen_list_2_item14;
	lv_obj_t *screen_list_2_item15;
	lv_obj_t *screen_list_2_item16;
	lv_obj_t *screen_list_2_item17;
	lv_obj_t *screen_list_2_item18;
	lv_obj_t *screen_list_2_item19;
	lv_obj_t *screen_list_2_item20;
	lv_obj_t *screen_btn_3;
	lv_obj_t *screen_btn_3_label;
	lv_obj_t *screen_slider_1;
	lv_obj_t *screen_label_16;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, int32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_screen(lv_ui *ui);
LV_IMG_DECLARE(_second_needle_2_alpha_110x5);
LV_IMG_DECLARE(_hour_needle_white_alpha_70x8);
LV_IMG_DECLARE(_min_needle_white_alpha_100x8);
LV_IMG_DECLARE(_RGB_R_alpha_280x456);
LV_IMG_DECLARE(_RGB_G_alpha_280x456);
LV_IMG_DECLARE(_RGB_B_alpha_280x456);
LV_IMG_DECLARE(_ble_5_alpha_100x90);
LV_IMG_DECLARE(_wifi__alpha_100x90);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_bluetooth_alpha_20x16);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x16);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);
LV_IMG_DECLARE(_wi_alpha_20x20);

LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_18)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_montserratMedium_15)
LV_FONT_DECLARE(lv_font_montserratMedium_25)


#ifdef __cplusplus
}
#endif
#endif
