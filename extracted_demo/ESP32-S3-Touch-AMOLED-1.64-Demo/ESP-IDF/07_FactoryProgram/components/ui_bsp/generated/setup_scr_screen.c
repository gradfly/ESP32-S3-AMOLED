/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_screen(lv_ui *ui)
{
    //Write codes screen
    ui->screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen, 280, 456);
    lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_carousel_1
    ui->screen_carousel_1 = lv_carousel_create(ui->screen);
    lv_carousel_set_element_width(ui->screen_carousel_1, 280);
    lv_obj_set_size(ui->screen_carousel_1, 280, 456);
    ui->screen_carousel_1_element_1 = lv_carousel_add_element(ui->screen_carousel_1, 0);
    ui->screen_carousel_1_element_2 = lv_carousel_add_element(ui->screen_carousel_1, 1);
    ui->screen_carousel_1_element_3 = lv_carousel_add_element(ui->screen_carousel_1, 2);
    ui->screen_carousel_1_element_4 = lv_carousel_add_element(ui->screen_carousel_1, 3);
    lv_obj_set_pos(ui->screen_carousel_1, 0, 0);
    lv_obj_set_size(ui->screen_carousel_1, 280, 456);
    lv_obj_set_scrollbar_mode(ui->screen_carousel_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_carousel_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_carousel_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_carousel_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_carousel_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_carousel_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_carousel_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_carousel_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_carousel_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_carousel_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_carousel_1_extra_element_items_default
    static lv_style_t style_screen_carousel_1_extra_element_items_default;
    ui_init_style(&style_screen_carousel_1_extra_element_items_default);

    lv_style_set_bg_opa(&style_screen_carousel_1_extra_element_items_default, 0);
    lv_style_set_outline_width(&style_screen_carousel_1_extra_element_items_default, 0);
    lv_style_set_border_width(&style_screen_carousel_1_extra_element_items_default, 0);
    lv_style_set_radius(&style_screen_carousel_1_extra_element_items_default, 5);
    lv_style_set_shadow_width(&style_screen_carousel_1_extra_element_items_default, 0);
    lv_obj_add_style(ui->screen_carousel_1_element_4, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_carousel_1_element_3, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_carousel_1_element_2, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_carousel_1_element_1, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_FOCUSED for &style_screen_carousel_1_extra_element_items_focused
    static lv_style_t style_screen_carousel_1_extra_element_items_focused;
    ui_init_style(&style_screen_carousel_1_extra_element_items_focused);

    lv_style_set_bg_opa(&style_screen_carousel_1_extra_element_items_focused, 0);
    lv_style_set_outline_width(&style_screen_carousel_1_extra_element_items_focused, 0);
    lv_style_set_border_width(&style_screen_carousel_1_extra_element_items_focused, 0);
    lv_style_set_radius(&style_screen_carousel_1_extra_element_items_focused, 5);
    lv_style_set_shadow_width(&style_screen_carousel_1_extra_element_items_focused, 0);
    lv_obj_add_style(ui->screen_carousel_1_element_4, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);
    lv_obj_add_style(ui->screen_carousel_1_element_3, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);
    lv_obj_add_style(ui->screen_carousel_1_element_2, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);
    lv_obj_add_style(ui->screen_carousel_1_element_1, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);



    //Write codes screen_cont_4
    ui->screen_cont_4 = lv_obj_create(ui->screen_carousel_1_element_1);
    lv_obj_set_pos(ui->screen_cont_4, 0, 0);
    lv_obj_set_size(ui->screen_cont_4, 280, 456);
    lv_obj_set_scrollbar_mode(ui->screen_cont_4, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->screen_cont_4, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_cont_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_cont_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_cont_4, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_cont_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_cont_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_3
    ui->screen_img_3 = lv_img_create(ui->screen_cont_4);
    lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_img_3, &_second_needle_2_alpha_110x5);
    lv_img_set_pivot(ui->screen_img_3, 13,2);
    lv_img_set_angle(ui->screen_img_3, 1780);
    lv_obj_set_pos(ui->screen_img_3, 128, 231);
    lv_obj_set_size(ui->screen_img_3, 110, 5);

    //Write style for screen_img_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_3, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_1
    ui->screen_img_1 = lv_img_create(ui->screen_cont_4);
    lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_img_1, &_hour_needle_white_alpha_70x8);
    lv_img_set_pivot(ui->screen_img_1, 3,4);
    lv_img_set_angle(ui->screen_img_1, -840);
    lv_obj_set_pos(ui->screen_img_1, 140, 228);
    lv_obj_set_size(ui->screen_img_1, 70, 8);

    //Write style for screen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_2
    ui->screen_img_2 = lv_img_create(ui->screen_cont_4);
    lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_img_2, &_min_needle_white_alpha_100x8);
    lv_img_set_pivot(ui->screen_img_2, 3,4);
    lv_img_set_angle(ui->screen_img_2, 100);
    lv_obj_set_pos(ui->screen_img_2, 140, 228);
    lv_obj_set_size(ui->screen_img_2, 100, 8);

    //Write style for screen_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_cont_5
    ui->screen_cont_5 = lv_obj_create(ui->screen_carousel_1_element_1);
    lv_obj_set_pos(ui->screen_cont_5, 0, 0);
    lv_obj_set_size(ui->screen_cont_5, 280, 456);
    lv_obj_set_scrollbar_mode(ui->screen_cont_5, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_cont_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_cont_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_cont_5, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_cont_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_4
    ui->screen_img_4 = lv_img_create(ui->screen_cont_5);
    lv_obj_add_flag(ui->screen_img_4, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_img_4, &_RGB_R_alpha_280x456);
    lv_img_set_pivot(ui->screen_img_4, 50,50);
    lv_img_set_angle(ui->screen_img_4, 0);
    lv_obj_set_pos(ui->screen_img_4, 0, 0);
    lv_obj_set_size(ui->screen_img_4, 280, 456);

    //Write style for screen_img_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_4, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_5
    ui->screen_img_5 = lv_img_create(ui->screen_cont_5);
    lv_obj_add_flag(ui->screen_img_5, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_img_5, &_RGB_G_alpha_280x456);
    lv_img_set_pivot(ui->screen_img_5, 50,50);
    lv_img_set_angle(ui->screen_img_5, 0);
    lv_obj_set_pos(ui->screen_img_5, 0, 0);
    lv_obj_set_size(ui->screen_img_5, 280, 456);
    lv_obj_add_flag(ui->screen_img_5, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_img_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_5, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_6
    ui->screen_img_6 = lv_img_create(ui->screen_cont_5);
    lv_obj_add_flag(ui->screen_img_6, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_img_6, &_RGB_B_alpha_280x456);
    lv_img_set_pivot(ui->screen_img_6, 50,50);
    lv_img_set_angle(ui->screen_img_6, 0);
    lv_obj_set_pos(ui->screen_img_6, 0, 0);
    lv_obj_set_size(ui->screen_img_6, 280, 456);
    lv_obj_add_flag(ui->screen_img_6, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_img_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_6, true, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes screen_label_1
    ui->screen_label_1 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_1, "SD_Card : ");
    lv_label_set_long_mode(ui->screen_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_1, 30, 162);
    lv_obj_set_size(ui->screen_label_1, 104, 15);

    //Write style for screen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_2
    ui->screen_label_2 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_2, "ADC_Voltage : ");
    lv_label_set_long_mode(ui->screen_label_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_2, 30, 192);
    lv_obj_set_size(ui->screen_label_2, 150, 15);

    //Write style for screen_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_2, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_3
    ui->screen_label_3 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_3, "Flash : ");
    lv_label_set_long_mode(ui->screen_label_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_3, 30, 102);
    lv_obj_set_size(ui->screen_label_3, 74, 15);

    //Write style for screen_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_3, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_4
    ui->screen_label_4 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_4, "PSRAM : ");
    lv_label_set_long_mode(ui->screen_label_4, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_4, 30, 132);
    lv_obj_set_size(ui->screen_label_4, 104, 15);

    //Write style for screen_label_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_4, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_5
    ui->screen_label_5 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_5, "16M");
    lv_label_set_long_mode(ui->screen_label_5, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_5, 105, 102);
    lv_obj_set_size(ui->screen_label_5, 104, 15);

    //Write style for screen_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_5, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_5, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_7
    ui->screen_label_7 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_7, "NULL");
    lv_label_set_long_mode(ui->screen_label_7, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_7, 132, 162);
    lv_obj_set_size(ui->screen_label_7, 104, 15);

    //Write style for screen_label_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_7, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_7, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_7, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_7, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_8
    ui->screen_label_8 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_8, "3.11V");
    lv_label_set_long_mode(ui->screen_label_8, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_8, 177, 193);
    lv_obj_set_size(ui->screen_label_8, 62, 15);

    //Write style for screen_label_8, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_8, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_8, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_8, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_8, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_6
    ui->screen_label_6 = lv_label_create(ui->screen_carousel_1_element_2);
    lv_label_set_text(ui->screen_label_6, "8M");
    lv_label_set_long_mode(ui->screen_label_6, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_6, 120, 132);
    lv_obj_set_size(ui->screen_label_6, 104, 15);

    //Write style for screen_label_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_6, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_6, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_spangroup_1
    ui->screen_spangroup_1 = lv_spangroup_create(ui->screen_carousel_1_element_2);
    lv_spangroup_set_align(ui->screen_spangroup_1, LV_TEXT_ALIGN_LEFT);
    lv_spangroup_set_overflow(ui->screen_spangroup_1, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(ui->screen_spangroup_1, LV_SPAN_MODE_BREAK);
    //create span
    ui->screen_spangroup_1_span = lv_spangroup_new_span(ui->screen_spangroup_1);
    lv_span_set_text(ui->screen_spangroup_1_span, "hello");
    lv_style_set_text_color(&ui->screen_spangroup_1_span->style, lv_color_hex(0xffffff));
    lv_style_set_text_decor(&ui->screen_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->screen_spangroup_1_span->style, &lv_font_montserratMedium_16);
    lv_obj_set_pos(ui->screen_spangroup_1, 30, 222);
    lv_obj_set_size(ui->screen_spangroup_1, 226, 132);

    //Write style state: LV_STATE_DEFAULT for &style_screen_spangroup_1_main_main_default
    static lv_style_t style_screen_spangroup_1_main_main_default;
    ui_init_style(&style_screen_spangroup_1_main_main_default);

    lv_style_set_border_width(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_radius(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_top(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_right(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_bottom(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_left(&style_screen_spangroup_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_screen_spangroup_1_main_main_default, 0);
    lv_obj_add_style(ui->screen_spangroup_1, &style_screen_spangroup_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_spangroup_refr_mode(ui->screen_spangroup_1);



    //Write codes screen_cont_2
    ui->screen_cont_2 = lv_obj_create(ui->screen_carousel_1_element_3);
    lv_obj_set_pos(ui->screen_cont_2, 5, 130);
    lv_obj_set_size(ui->screen_cont_2, 266, 225);
    lv_obj_set_scrollbar_mode(ui->screen_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_cont_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_15
    ui->screen_label_15 = lv_label_create(ui->screen_cont_2);
    lv_label_set_text(ui->screen_label_15, "Communication test : ");
    lv_label_set_long_mode(ui->screen_label_15, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_15, 0, 0);
    lv_obj_set_size(ui->screen_label_15, 255, 26);

    //Write style for screen_label_15, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_15, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_15, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_15, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_15, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_15, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_imgbtn_1
    ui->screen_imgbtn_1 = lv_imgbtn_create(ui->screen_cont_2);
    lv_obj_add_flag(ui->screen_imgbtn_1, LV_OBJ_FLAG_CHECKABLE);
    lv_imgbtn_set_src(ui->screen_imgbtn_1, LV_IMGBTN_STATE_RELEASED, NULL, &_ble_5_alpha_100x90, NULL);
    ui->screen_imgbtn_1_label = lv_label_create(ui->screen_imgbtn_1);
    lv_label_set_text(ui->screen_imgbtn_1_label, "");
    lv_label_set_long_mode(ui->screen_imgbtn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_imgbtn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_imgbtn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(ui->screen_imgbtn_1, 10, 88);
    lv_obj_set_size(ui->screen_imgbtn_1, 100, 90);

    //Write style for screen_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_imgbtn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_imgbtn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_imgbtn_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_img_recolor_opa(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_img_opa(ui->screen_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->screen_imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->screen_imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->screen_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for screen_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_img_recolor_opa(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_img_opa(ui->screen_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->screen_imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->screen_imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->screen_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for screen_imgbtn_1, Part: LV_PART_MAIN, State: LV_IMGBTN_STATE_RELEASED.
    lv_obj_set_style_img_recolor_opa(ui->screen_imgbtn_1, 0, LV_PART_MAIN|LV_IMGBTN_STATE_RELEASED);
    lv_obj_set_style_img_opa(ui->screen_imgbtn_1, 255, LV_PART_MAIN|LV_IMGBTN_STATE_RELEASED);

    //Write codes screen_imgbtn_2
    ui->screen_imgbtn_2 = lv_imgbtn_create(ui->screen_cont_2);
    lv_obj_add_flag(ui->screen_imgbtn_2, LV_OBJ_FLAG_CHECKABLE);
    lv_imgbtn_set_src(ui->screen_imgbtn_2, LV_IMGBTN_STATE_RELEASED, NULL, &_wifi__alpha_100x90, NULL);
    ui->screen_imgbtn_2_label = lv_label_create(ui->screen_imgbtn_2);
    lv_label_set_text(ui->screen_imgbtn_2_label, "");
    lv_label_set_long_mode(ui->screen_imgbtn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_imgbtn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_imgbtn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(ui->screen_imgbtn_2, 150, 89);
    lv_obj_set_size(ui->screen_imgbtn_2, 100, 90);

    //Write style for screen_imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_imgbtn_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_imgbtn_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_imgbtn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_imgbtn_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_img_recolor_opa(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_img_opa(ui->screen_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->screen_imgbtn_2, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->screen_imgbtn_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->screen_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for screen_imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_img_recolor_opa(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_img_opa(ui->screen_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->screen_imgbtn_2, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->screen_imgbtn_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->screen_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for screen_imgbtn_2, Part: LV_PART_MAIN, State: LV_IMGBTN_STATE_RELEASED.
    lv_obj_set_style_img_recolor_opa(ui->screen_imgbtn_2, 0, LV_PART_MAIN|LV_IMGBTN_STATE_RELEASED);
    lv_obj_set_style_img_opa(ui->screen_imgbtn_2, 255, LV_PART_MAIN|LV_IMGBTN_STATE_RELEASED);

    //Write codes screen_cont_1
    ui->screen_cont_1 = lv_obj_create(ui->screen_carousel_1_element_3);
    lv_obj_set_pos(ui->screen_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_cont_1, 280, 456);
    lv_obj_set_scrollbar_mode(ui->screen_cont_1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->screen_cont_1, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_cont_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_list_1
    ui->screen_list_1 = lv_list_create(ui->screen_cont_1);
    ui->screen_list_1_item0 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "ble1");
    ui->screen_list_1_item1 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "ble2");
    ui->screen_list_1_item2 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "ble3");
    ui->screen_list_1_item3 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "ble4");
    ui->screen_list_1_item4 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "ble5");
    ui->screen_list_1_item5 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "ble6");
    ui->screen_list_1_item6 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_6");
    ui->screen_list_1_item7 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_7");
    ui->screen_list_1_item8 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_8");
    ui->screen_list_1_item9 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_9");
    ui->screen_list_1_item10 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_10");
    ui->screen_list_1_item11 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_11");
    ui->screen_list_1_item12 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_12");
    ui->screen_list_1_item13 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_13");
    ui->screen_list_1_item14 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_14");
    ui->screen_list_1_item15 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_15");
    ui->screen_list_1_item16 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_16");
    ui->screen_list_1_item17 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_17");
    ui->screen_list_1_item18 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_18");
    ui->screen_list_1_item19 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_19");
    ui->screen_list_1_item20 = lv_list_add_btn(ui->screen_list_1, &_bluetooth_alpha_20x16, "save_20");
    lv_obj_set_pos(ui->screen_list_1, 1, 46);
    lv_obj_set_size(ui->screen_list_1, 172, 402);
    lv_obj_set_scrollbar_mode(ui->screen_list_1, LV_SCROLLBAR_MODE_OFF);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_1_main_main_default
    static lv_style_t style_screen_list_1_main_main_default;
    ui_init_style(&style_screen_list_1_main_main_default);

    lv_style_set_pad_top(&style_screen_list_1_main_main_default, 0);
    lv_style_set_pad_left(&style_screen_list_1_main_main_default, 0);
    lv_style_set_pad_right(&style_screen_list_1_main_main_default, 0);
    lv_style_set_pad_bottom(&style_screen_list_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_screen_list_1_main_main_default, 255);
    lv_style_set_bg_color(&style_screen_list_1_main_main_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_screen_list_1_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_screen_list_1_main_main_default, 0);
    lv_style_set_radius(&style_screen_list_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_screen_list_1_main_main_default, 0);
    lv_obj_add_style(ui->screen_list_1, &style_screen_list_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_1_main_scrollbar_default
    static lv_style_t style_screen_list_1_main_scrollbar_default;
    ui_init_style(&style_screen_list_1_main_scrollbar_default);

    lv_style_set_radius(&style_screen_list_1_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_screen_list_1_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_screen_list_1_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_list_1_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->screen_list_1, &style_screen_list_1_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_1_extra_btns_main_default
    static lv_style_t style_screen_list_1_extra_btns_main_default;
    ui_init_style(&style_screen_list_1_extra_btns_main_default);

    lv_style_set_pad_top(&style_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_screen_list_1_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_screen_list_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_screen_list_1_extra_btns_main_default, &lv_font_montserratMedium_16);
    lv_style_set_text_opa(&style_screen_list_1_extra_btns_main_default, 255);
    lv_style_set_radius(&style_screen_list_1_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_screen_list_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_screen_list_1_extra_btns_main_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_screen_list_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->screen_list_1_item20, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item19, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item18, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item17, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item16, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item15, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item14, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item13, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item12, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item11, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item10, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item9, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item8, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item7, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item6, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item5, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item4, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item3, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item2, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item1, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_1_item0, &style_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_1_extra_texts_main_default
    static lv_style_t style_screen_list_1_extra_texts_main_default;
    ui_init_style(&style_screen_list_1_extra_texts_main_default);

    lv_style_set_pad_top(&style_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_screen_list_1_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_screen_list_1_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_screen_list_1_extra_texts_main_default, &lv_font_montserratMedium_15);
    lv_style_set_text_opa(&style_screen_list_1_extra_texts_main_default, 255);
    lv_style_set_radius(&style_screen_list_1_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_screen_list_1_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_screen_list_1_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_screen_list_1_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_list_1_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //Write codes screen_label_13
    ui->screen_label_13 = lv_label_create(ui->screen_cont_1);
    lv_label_set_text(ui->screen_label_13, "BLE : ");
    lv_label_set_long_mode(ui->screen_label_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_13, 2, 12);
    lv_obj_set_size(ui->screen_label_13, 100, 26);

    //Write style for screen_label_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_13, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_13, &lv_font_montserratMedium_25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_13, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_btn_1
    ui->screen_btn_1 = lv_btn_create(ui->screen_cont_1);
    ui->screen_btn_1_label = lv_label_create(ui->screen_btn_1);
    lv_label_set_text(ui->screen_btn_1_label, "Scan");
    lv_label_set_long_mode(ui->screen_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_1_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_btn_1, 182, 256);
    lv_obj_set_size(ui->screen_btn_1, 90, 180);

    //Write style for screen_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_btn_1, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_1, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_btn_4
    ui->screen_btn_4 = lv_btn_create(ui->screen_cont_1);
    ui->screen_btn_4_label = lv_label_create(ui->screen_btn_4);
    lv_label_set_text(ui->screen_btn_4_label, "exit");
    lv_label_set_long_mode(ui->screen_btn_4_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_4_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_4_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_btn_4, 182, 54);
    lv_obj_set_size(ui->screen_btn_4, 90, 180);

    //Write style for screen_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_btn_4, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_btn_4, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_4, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_4, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_cont_3
    ui->screen_cont_3 = lv_obj_create(ui->screen_carousel_1_element_3);
    lv_obj_set_pos(ui->screen_cont_3, 0, 0);
    lv_obj_set_size(ui->screen_cont_3, 280, 456);
    lv_obj_set_scrollbar_mode(ui->screen_cont_3, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_cont_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_cont_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_cont_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_cont_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_btn_2
    ui->screen_btn_2 = lv_btn_create(ui->screen_cont_3);
    ui->screen_btn_2_label = lv_label_create(ui->screen_btn_2);
    lv_label_set_text(ui->screen_btn_2_label, "Scan");
    lv_label_set_long_mode(ui->screen_btn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_2_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_btn_2, 182, 256);
    lv_obj_set_size(ui->screen_btn_2, 90, 180);

    //Write style for screen_btn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_btn_2, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_btn_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_2, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_2, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_14
    ui->screen_label_14 = lv_label_create(ui->screen_cont_3);
    lv_label_set_text(ui->screen_label_14, "WIFI : ");
    lv_label_set_long_mode(ui->screen_label_14, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_14, 2, 9);
    lv_obj_set_size(ui->screen_label_14, 100, 26);

    //Write style for screen_label_14, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_14, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_14, &lv_font_montserratMedium_25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_14, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_14, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_14, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_list_2
    ui->screen_list_2 = lv_list_create(ui->screen_cont_3);
    ui->screen_list_2_item0 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "ble");
    ui->screen_list_2_item1 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "ble2");
    ui->screen_list_2_item2 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "ble3");
    ui->screen_list_2_item3 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "ble4");
    ui->screen_list_2_item4 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_4");
    ui->screen_list_2_item5 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_5");
    ui->screen_list_2_item6 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_6");
    ui->screen_list_2_item7 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_7");
    ui->screen_list_2_item8 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_8");
    ui->screen_list_2_item9 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_9");
    ui->screen_list_2_item10 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_10");
    ui->screen_list_2_item11 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x16, "save_11");
    ui->screen_list_2_item12 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_12");
    ui->screen_list_2_item13 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_13");
    ui->screen_list_2_item14 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_14");
    ui->screen_list_2_item15 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_15");
    ui->screen_list_2_item16 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_16");
    ui->screen_list_2_item17 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_17");
    ui->screen_list_2_item18 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_18");
    ui->screen_list_2_item19 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_19");
    ui->screen_list_2_item20 = lv_list_add_btn(ui->screen_list_2, &_wi_alpha_20x20, "save_20");
    lv_obj_set_pos(ui->screen_list_2, 1, 46);
    lv_obj_set_size(ui->screen_list_2, 172, 388);
    lv_obj_set_scrollbar_mode(ui->screen_list_2, LV_SCROLLBAR_MODE_OFF);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_2_main_main_default
    static lv_style_t style_screen_list_2_main_main_default;
    ui_init_style(&style_screen_list_2_main_main_default);

    lv_style_set_pad_top(&style_screen_list_2_main_main_default, 0);
    lv_style_set_pad_left(&style_screen_list_2_main_main_default, 0);
    lv_style_set_pad_right(&style_screen_list_2_main_main_default, 0);
    lv_style_set_pad_bottom(&style_screen_list_2_main_main_default, 0);
    lv_style_set_bg_opa(&style_screen_list_2_main_main_default, 255);
    lv_style_set_bg_color(&style_screen_list_2_main_main_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_screen_list_2_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_screen_list_2_main_main_default, 0);
    lv_style_set_radius(&style_screen_list_2_main_main_default, 0);
    lv_style_set_shadow_width(&style_screen_list_2_main_main_default, 0);
    lv_obj_add_style(ui->screen_list_2, &style_screen_list_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_2_main_scrollbar_default
    static lv_style_t style_screen_list_2_main_scrollbar_default;
    ui_init_style(&style_screen_list_2_main_scrollbar_default);

    lv_style_set_radius(&style_screen_list_2_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_screen_list_2_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_screen_list_2_main_scrollbar_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_screen_list_2_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->screen_list_2, &style_screen_list_2_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_2_extra_btns_main_default
    static lv_style_t style_screen_list_2_extra_btns_main_default;
    ui_init_style(&style_screen_list_2_extra_btns_main_default);

    lv_style_set_pad_top(&style_screen_list_2_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_screen_list_2_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_screen_list_2_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_list_2_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_screen_list_2_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_screen_list_2_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_screen_list_2_extra_btns_main_default, &lv_font_montserratMedium_16);
    lv_style_set_text_opa(&style_screen_list_2_extra_btns_main_default, 255);
    lv_style_set_radius(&style_screen_list_2_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_screen_list_2_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_screen_list_2_extra_btns_main_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_screen_list_2_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->screen_list_2_item20, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item19, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item18, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item17, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item16, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item15, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item14, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item13, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item12, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item11, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item10, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item9, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item8, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item7, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item6, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item5, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item4, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item3, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item2, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item1, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_list_2_item0, &style_screen_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_list_2_extra_texts_main_default
    static lv_style_t style_screen_list_2_extra_texts_main_default;
    ui_init_style(&style_screen_list_2_extra_texts_main_default);

    lv_style_set_pad_top(&style_screen_list_2_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_screen_list_2_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_screen_list_2_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_list_2_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_screen_list_2_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_screen_list_2_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_screen_list_2_extra_texts_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_screen_list_2_extra_texts_main_default, 255);
    lv_style_set_radius(&style_screen_list_2_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_screen_list_2_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_screen_list_2_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_screen_list_2_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_list_2_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //Write codes screen_btn_3
    ui->screen_btn_3 = lv_btn_create(ui->screen_cont_3);
    ui->screen_btn_3_label = lv_label_create(ui->screen_btn_3);
    lv_label_set_text(ui->screen_btn_3_label, "exit");
    lv_label_set_long_mode(ui->screen_btn_3_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_3_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_btn_3, 182, 54);
    lv_obj_set_size(ui->screen_btn_3, 90, 180);

    //Write style for screen_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_btn_3, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_btn_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_3, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_3, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes screen_slider_1
    ui->screen_slider_1 = lv_slider_create(ui->screen_carousel_1_element_4);
    lv_slider_set_range(ui->screen_slider_1, 0, 255);
    lv_slider_set_mode(ui->screen_slider_1, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->screen_slider_1, 255, LV_ANIM_OFF);
    lv_obj_set_pos(ui->screen_slider_1, 17, 265);
    lv_obj_set_size(ui->screen_slider_1, 229, 24);

    //Write style for screen_slider_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_slider_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_slider_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_slider_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_slider_1, 50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->screen_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_slider_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_slider_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_slider_1, lv_color_hex(0xffffff), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_slider_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_slider_1, 50, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for screen_slider_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_slider_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_slider_1, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_slider_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_slider_1, 50, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes screen_label_16
    ui->screen_label_16 = lv_label_create(ui->screen_carousel_1_element_4);
    lv_label_set_text(ui->screen_label_16, "Slide the silder to adjust the brightness:");
    lv_label_set_long_mode(ui->screen_label_16, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_label_16, 8, 141);
    lv_obj_set_size(ui->screen_label_16, 264, 31);

    //Write style for screen_label_16, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_16, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_16, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_16, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_16, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen);

}
