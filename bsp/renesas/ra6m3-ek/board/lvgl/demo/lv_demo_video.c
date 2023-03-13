/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2023-03-09     Rbb666        First version
 */
#include "lvgl.h"
#include <rtthread.h>
#include <lcd_port.h>
#include "drv_jpeg.h"

#include "lv_file_explorer.h"

#define JPEG_WIDTH  400
#define JPEG_HEIGHT 240

#define MY_CLASS &lv_media_class

typedef enum {
    LV_MEDIA_STATE_NORMAL,
    LV_MEDIA_STATE_PAUSE,
    LV_MEDIA_STATE_PLAY,
    LV_MEDIA_STATE_NEXT,
    LV_MEDIA_STATE_PREV,
} lv_mdeia_state_t;

typedef enum {
    LV_FILE_EXPLORER_OPEN,
    LV_FILE_EXPLORER_CLOSE,
} lv_file_btn_starte_t;

typedef struct
{
    lv_obj_t obj;
    char *cur_fn;
    lv_mdeia_state_t state;
} lv_media_obj_t;

struct q_rx_msg
{
    char *data;
    rt_size_t size;
};

const lv_obj_class_t lv_media_class = 
{
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_media_obj_t),
    .base_class = &lv_obj_class
};

static lv_obj_t *avi_obj;
static lv_obj_t *file_explorer_panel;
static lv_obj_t *file_ImgButton;
static rt_bool_t btn_state_change = RT_FALSE;
extern rt_mq_t video_msg_mq;

static uint16_t lv_show_buffer[JPEG_WIDTH * JPEG_HEIGHT] BSP_ALIGN_VARIABLE(16);

void lv_media_set_state(lv_obj_t *obj, lv_mdeia_state_t state)
{
    lv_media_obj_t *media = (lv_media_obj_t *)obj;
    media->state = state;
}

lv_mdeia_state_t lv_media_get_state(lv_obj_t *obj, lv_mdeia_state_t state)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_media_obj_t *media = (lv_media_obj_t *)obj;

    return media->state;
}

void lv_media_set_fn(lv_obj_t *obj, char *fn)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_media_obj_t *fd = (lv_media_obj_t *)obj;

    uint16_t fn_len = strlen(fn);

    if ((!fn) || (fn_len == 0)) return;

    if (fd->cur_fn)
    {
        lv_mem_free(fd->cur_fn);
        fd->cur_fn = NULL;
    }

    fd->cur_fn = lv_mem_alloc(fn_len + 1);
    LV_ASSERT_MALLOC(fd->cur_fn);

    if (fd->cur_fn == NULL) return;
    strcpy(fd->cur_fn, fn);
}

char *lv_media_get_fn(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_media_obj_t *media = (lv_media_obj_t *)obj;

    return media->cur_fn;
}

static void file_explorer_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *file_explorer = lv_event_get_target(e);
    lv_obj_t *file = lv_event_get_user_data(e);
    
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        const char *path = lv_file_explorer_get_current_path(file_explorer);
        const char *fn = lv_file_explorer_get_selected_file_name(file_explorer);

        uint16_t path_len = strlen(path);
        uint16_t fn_len = strlen(fn);

        if ((path_len + fn_len) <= LV_FILE_EXPLORER_PATH_MAX_LEN)
        {
            char sel_fn[LV_FILE_EXPLORER_PATH_MAX_LEN];

            strcpy(sel_fn, path);
            if (*(path + path_len) != '/')
                strcat(sel_fn, "/");
            strcat(sel_fn, fn);
            rt_kprintf("path:%s\n", sel_fn);

            lv_media_set_fn(file, sel_fn);
            /* send event to take back file panel */
            lv_event_send(file_ImgButton, LV_EVENT_RELEASED, NULL);
            
            rt_kprintf("send len:%d\n", strlen(sel_fn));
            
            struct q_rx_msg msg;
            msg.data = sel_fn;
            msg.size = strlen(sel_fn);

            rt_mq_send(video_msg_mq, &msg, sizeof(msg));
        }
    }
}

//
void _ui_anim_callback_set_x(lv_anim_t * a, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)a->user_data, v);
}

void _ui_anim_callback_set_y(lv_anim_t * a, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)a->user_data, v);
}

void _ui_anim_callback_set_width(lv_anim_t * a, int32_t v)
{
    lv_obj_set_width((lv_obj_t *)a->user_data, v);
}

void _ui_anim_callback_set_height(lv_anim_t * a, int32_t v)
{
    lv_obj_set_height((lv_obj_t *)a->user_data, v);
}

void roll_out_Animation(lv_obj_t * TargetObject, int delay)
{
    lv_anim_t PropertyAnimation_0;
    lv_anim_init(&PropertyAnimation_0);
    lv_anim_set_time(&PropertyAnimation_0, 600);
    lv_anim_set_user_data(&PropertyAnimation_0, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_0, _ui_anim_callback_set_y);
    lv_anim_set_values(&PropertyAnimation_0, -200, 0);
    lv_anim_set_path_cb(&PropertyAnimation_0, lv_anim_path_ease_out);
    lv_anim_set_delay(&PropertyAnimation_0, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_0, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_0, 0);
    lv_anim_set_early_apply(&PropertyAnimation_0, false);
    lv_anim_start(&PropertyAnimation_0);
    lv_anim_t PropertyAnimation_1;
    lv_anim_init(&PropertyAnimation_1);
    lv_anim_set_time(&PropertyAnimation_1, 600);
    lv_anim_set_user_data(&PropertyAnimation_1, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_1, _ui_anim_callback_set_x);
    lv_anim_set_values(&PropertyAnimation_1, 300, 0);
    lv_anim_set_path_cb(&PropertyAnimation_1, lv_anim_path_ease_out);
    lv_anim_set_delay(&PropertyAnimation_1, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_1, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_1, 0);
    lv_anim_set_early_apply(&PropertyAnimation_1, false);
    lv_anim_start(&PropertyAnimation_1);
    lv_anim_t PropertyAnimation_2;
    lv_anim_init(&PropertyAnimation_2);
    lv_anim_set_time(&PropertyAnimation_2, 600);
    lv_anim_set_user_data(&PropertyAnimation_2, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_2, _ui_anim_callback_set_width);
    lv_anim_set_values(&PropertyAnimation_2, 0, 370);
    lv_anim_set_path_cb(&PropertyAnimation_2, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_2, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_2, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_2, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_2, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_2, 0);
    lv_anim_set_early_apply(&PropertyAnimation_2, false);
    lv_anim_start(&PropertyAnimation_2);
    lv_anim_t PropertyAnimation_3;
    lv_anim_init(&PropertyAnimation_3);
    lv_anim_set_time(&PropertyAnimation_3, 600);
    lv_anim_set_user_data(&PropertyAnimation_3, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_3, _ui_anim_callback_set_height);
    lv_anim_set_values(&PropertyAnimation_3, 0, 200);
    lv_anim_set_path_cb(&PropertyAnimation_3, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_3, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_3, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_3, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_3, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_3, 0);
    lv_anim_set_early_apply(&PropertyAnimation_3, false);
    lv_anim_start(&PropertyAnimation_3);
}

void take_back_Animation(lv_obj_t * TargetObject, int delay)
{
    lv_anim_t PropertyAnimation_0;
    lv_anim_init(&PropertyAnimation_0);
    lv_anim_set_time(&PropertyAnimation_0, 600);
    lv_anim_set_user_data(&PropertyAnimation_0, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_0, _ui_anim_callback_set_x);
    lv_anim_set_values(&PropertyAnimation_0, 0, 300);
    lv_anim_set_path_cb(&PropertyAnimation_0, lv_anim_path_ease_out);
    lv_anim_set_delay(&PropertyAnimation_0, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_0, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_0, 0);
    lv_anim_set_early_apply(&PropertyAnimation_0, false);
    lv_anim_start(&PropertyAnimation_0);
    lv_anim_t PropertyAnimation_1;
    lv_anim_init(&PropertyAnimation_1);
    lv_anim_set_time(&PropertyAnimation_1, 600);
    lv_anim_set_user_data(&PropertyAnimation_1, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_1, _ui_anim_callback_set_y);
    lv_anim_set_values(&PropertyAnimation_1, 0, -200);
    lv_anim_set_path_cb(&PropertyAnimation_1, lv_anim_path_ease_out);
    lv_anim_set_delay(&PropertyAnimation_1, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_1, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_1, 0);
    lv_anim_set_early_apply(&PropertyAnimation_1, false);
    lv_anim_start(&PropertyAnimation_1);
    lv_anim_t PropertyAnimation_2;
    lv_anim_init(&PropertyAnimation_2);
    lv_anim_set_time(&PropertyAnimation_2, 600);
    lv_anim_set_user_data(&PropertyAnimation_2, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_2, _ui_anim_callback_set_width);
    lv_anim_set_values(&PropertyAnimation_2, 370, 0);
    lv_anim_set_path_cb(&PropertyAnimation_2, lv_anim_path_ease_out);
    lv_anim_set_delay(&PropertyAnimation_2, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_2, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_2, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_2, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_2, 0);
    lv_anim_set_early_apply(&PropertyAnimation_2, false);
    lv_anim_start(&PropertyAnimation_2);
    lv_anim_t PropertyAnimation_3;
    lv_anim_init(&PropertyAnimation_3);
    lv_anim_set_time(&PropertyAnimation_3, 600);
    lv_anim_set_user_data(&PropertyAnimation_3, TargetObject);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_3, _ui_anim_callback_set_height);
    lv_anim_set_values(&PropertyAnimation_3, 200, 0);
    lv_anim_set_path_cb(&PropertyAnimation_3, lv_anim_path_ease_out);
    lv_anim_set_delay(&PropertyAnimation_3, delay + 0);
    lv_anim_set_playback_time(&PropertyAnimation_3, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_3, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_3, 0);
    lv_anim_set_repeat_delay(&PropertyAnimation_3, 0);
    lv_anim_set_early_apply(&PropertyAnimation_3, false);
    lv_anim_start(&PropertyAnimation_3);
}

void file_ImgButton_event(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_RELEASED)
    {
        btn_state_change = !btn_state_change;

        btn_state_change == LV_FILE_EXPLORER_OPEN ?         \
            roll_out_Animation(file_explorer_panel, 0) :    \
            take_back_Animation(file_explorer_panel, 0);    \
    }
}

lv_obj_t *lv_media_page_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void file_explorer_create(lv_obj_t *parent)
{
    lv_obj_set_style_border_width(lv_scr_act(), 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_size(parent, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_center(parent);

    file_explorer_panel = lv_file_explorer_create(parent);
    lv_obj_set_size(file_explorer_panel, 370, 200);
    lv_obj_center(file_explorer_panel);

    lv_file_explorer_set_sort(file_explorer_panel, LV_EXPLORER_SORT_NONE);
    lv_file_explorer_open_dir(file_explorer_panel, "/");

    lv_obj_add_event_cb(file_explorer_panel, file_explorer_event_cb, LV_EVENT_VALUE_CHANGED, parent);
}

void file_ImgButton_create(lv_obj_t *parent)
{    
    LV_IMG_DECLARE(file_icon);
    
    file_ImgButton = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(file_ImgButton, LV_IMGBTN_STATE_RELEASED, NULL, &file_icon, NULL);
    lv_obj_set_size(file_ImgButton, 62, 62);
    lv_obj_align(file_ImgButton, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_add_event_cb(file_ImgButton, file_ImgButton_event, LV_EVENT_ALL, NULL);
}

void lv_avi_window_create(lv_obj_t *parent)
{
    avi_obj = lv_img_create(parent);
    lv_obj_set_size(avi_obj, JPEG_WIDTH, JPEG_HEIGHT);
    lv_obj_align(avi_obj, LV_ALIGN_CENTER, 0, 0);
}

void lv_avi_player_draw(int32_t x, int32_t y, void *pInBuffer, int32_t xSize, int32_t ySize)
{
    static lv_img_dsc_t img_dsc =
    {
        .header.always_zero = 0,
        .header.w = JPEG_WIDTH,
        .header.h = JPEG_HEIGHT,
        .header.cf = LV_IMG_CF_TRUE_COLOR,
        .data_size = JPEG_WIDTH * JPEG_HEIGHT * sizeof(lv_color16_t),
        .data = NULL,
    };

    uint16_t *fbp16 = (uint16_t *)lv_show_buffer;
    long int location = 0;

    location = x + y * JPEG_WIDTH;
    rt_memcpy(&fbp16[location], pInBuffer, (ySize * JPEG_WIDTH * sizeof(lv_color16_t)));

    if (y == JPEG_HEIGHT - 16)
    {
        img_dsc.data_size = ySize * xSize * sizeof(lv_color16_t);
        img_dsc.data = (const uint8_t *)fbp16;
        lv_img_set_src(avi_obj, &img_dsc);
    }
}

void lv_video_gui_init(void)
{
    lv_obj_t *win_obj;
    win_obj = lv_media_page_create(lv_scr_act());
    lv_avi_window_create(win_obj);
    file_explorer_create(win_obj);
    file_ImgButton_create(win_obj);
}
