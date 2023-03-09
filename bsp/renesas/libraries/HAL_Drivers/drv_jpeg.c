/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-12-15     Rbb666       the first version
 */
#include <rtthread.h>
#include "rthw.h"

#include "drv_jpeg.h"

#ifdef BSP_USING_JPEG

#define DCODE_BUFFER_SIZE           (30 * 1024)
#define JPEG_NUMBYTES_OUTBUFFER     (DCODE_BUFFER_SIZE)
#define JPEG_TIMEOUT                (100)

#define JPEG_WIDTH  400
#define JPEG_HEIGHT 240

volatile static jpeg_status_t g_jpeg_status = JPEG_STATUS_NONE;

static uint8_t g_OutBuffer[JPEG_NUMBYTES_OUTBUFFER] BSP_ALIGN_VARIABLE(8);

static rt_sem_t _SemaphoreJPEG = RT_NULL;

// jpeg and lvgl can only select one
rt_weak void _ra_port_display_callback(display_callback_args_t *p_args)
{
    if (DISPLAY_EVENT_LINE_DETECTION == p_args->event)
    {
    }
}

static int32_t _WaitForCallbackTimed(uint32_t TimeOut)
{
    return rt_sem_take(_SemaphoreJPEG, TimeOut) ? RT_ETIMEOUT : RT_EOK;
}

static void _DrawBitmap(int32_t x, int32_t y, void const *p, int32_t xSize, int32_t ySize)
{
    extern void LCDCONF_DrawJPEG(int32_t      x,
                                 int32_t      y,
                                 const void  *p,
                                 int32_t      xSize,
                                 int32_t      ySize);

    LCDCONF_DrawJPEG(x, y, p, xSize, ySize);
}

#include <lvgl.h>

static lv_obj_t *avi_obj = RT_NULL;
static lv_obj_t *win_obj = RT_NULL;

void lv_avi_create(void)
{
    win_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(win_obj, 480, 272);
    lv_obj_align(win_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_border_width(win_obj, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(win_obj, lv_color_black(), LV_STATE_DEFAULT);

    avi_obj = lv_img_create(win_obj);
    lv_obj_set_size(avi_obj, JPEG_WIDTH, JPEG_HEIGHT);
    lv_obj_align(avi_obj, LV_ALIGN_CENTER, 0, 0);
}

static uint16_t lv_show_buffer[JPEG_WIDTH * JPEG_HEIGHT] BSP_ALIGN_VARIABLE(16);
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

int JPEG_X_Draw(void *p, int x0, int y0)
{
    int32_t x;
    int32_t y;
    int32_t Error;
    int32_t xSize;
    int32_t ySize;
    int32_t OutBufferSize;
    uint32_t LinesDecoded;
    jpeg_color_space_t ColorSpace;

    void *pOutBuffer;
    void *_aInBuffer;

    Error = 0;
    xSize = 0;
    ySize = 0;
    x = x0;
    y = y0;
    g_jpeg_status = JPEG_STATUS_NONE;

    fsp_err_t err = FSP_SUCCESS;
    /* Initialize JPEG Codec Driver */
    err = R_JPEG_Open(&g_jpeg0_ctrl, &g_jpeg0_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* JPEG Codec initialization failed  */
        rt_kprintf("JPEG Codec driver initialization FAILED\n");
    }

    _aInBuffer = (uint8_t *)p;

    while (!(g_jpeg_status & JPEG_STATUS_ERROR))
    {
        /* Set in-buffer to get some information about the JPEG */
        if (R_JPEG_InputBufferSet(&g_jpeg0_ctrl, _aInBuffer,
                                  DCODE_BUFFER_SIZE) != FSP_SUCCESS)
        {
            Error = 2;
            break;
        }

        /* Wait for callback */
        if (_WaitForCallbackTimed(JPEG_TIMEOUT) == RT_ETIMEOUT)
        {
            Error = 3;
            break;
        }

        if (g_jpeg_status & JPEG_STATUS_IMAGE_SIZE_READY)
        {
            break;
        }

        /* Move pointer to next block of input data (if needed) */
        // _aInBuffer = (uint8_t *)((uint32_t) _aInBuffer + DCODE_BUFFER_SIZE);
    }

    if (!Error)
    {
        /* Get image dimensions */
        if (R_JPEG_DecodeImageSizeGet(&g_jpeg0_ctrl, (uint16_t *) &xSize,
                                      (uint16_t *) &ySize) != FSP_SUCCESS)
        {
            Error = 4;
        }
    }

    if (!Error)
    {
        /* Get color space and check dimensions accordingly */
        R_JPEG_DecodePixelFormatGet(&g_jpeg0_ctrl, &ColorSpace);
        if (g_jpeg_status & JPEG_STATUS_ERROR)
        {
            /* Image dimensions incompatible with JPEG Codec */
            Error = 5;
        }
    }

    if (!Error)
    {
        /* Set up out buffer */
        // xSize * 16 * 2
        OutBufferSize = xSize * 16 * 2;
        pOutBuffer    = (void *) g_OutBuffer;

        /* Set stride value */
        if (R_JPEG_DecodeHorizontalStrideSet(&g_jpeg0_ctrl, (uint32_t) xSize) != FSP_SUCCESS)
        {
            Error = 6;
        }
    }

    if (!Error)
    {
        /* Start decoding process by setting out-buffer */
        if (R_JPEG_OutputBufferSet(&g_jpeg0_ctrl, pOutBuffer, (uint32_t) OutBufferSize) != FSP_SUCCESS)
        {
            Error = 7;
        }
    }

    if (!Error)
    {
        while (!(g_jpeg_status & JPEG_STATUS_ERROR))
        {
            if (_WaitForCallbackTimed(JPEG_TIMEOUT) == RT_ETIMEOUT)
            {
                Error = 8;
                break;
            }

            if ((g_jpeg_status & JPEG_STATUS_OUTPUT_PAUSE) || (g_jpeg_status & JPEG_STATUS_OPERATION_COMPLETE))
            {
                /* Draw the JPEG work buffer to the framebuffer */
                R_JPEG_DecodeLinesDecodedGet(&g_jpeg0_ctrl, &LinesDecoded);

//                _DrawBitmap(x, y, (void const *) pOutBuffer, xSize, (int32_t) LinesDecoded);
                lv_avi_player_draw(x, y, (void *) pOutBuffer, xSize, (int32_t)LinesDecoded);

                y = y + (int32_t)LinesDecoded;

                if (!(g_jpeg_status & JPEG_STATUS_OPERATION_COMPLETE))
                {
                    /* Set the output buffer to the next 16-line block */
                    if (R_JPEG_OutputBufferSet(&g_jpeg0_ctrl, pOutBuffer,
                                               (uint32_t) OutBufferSize) != FSP_SUCCESS)
                    {
                        Error = 10;
                        break;
                    }
                }
            }

            if (g_jpeg_status & JPEG_STATUS_INPUT_PAUSE)
            {
                /* Get next block of input data */
                // _aInBuffer = (uint8_t *)((uint32_t) _aInBuffer + DCODE_BUFFER_SIZE);
                /* Set the new input buffer pointer */
                if (R_JPEG_InputBufferSet(&g_jpeg0_ctrl, (void *) _aInBuffer,
                                          DCODE_BUFFER_SIZE) != FSP_SUCCESS)
                {
                    Error = 9;
                    break;
                }
            }

            if (g_jpeg_status & JPEG_STATUS_OPERATION_COMPLETE)
            {
                break;
            }
        }
    }

    if ((g_jpeg_status & JPEG_STATUS_ERROR) && (!Error))
    {
        Error = 1;
    }

    R_JPEG_Close(&g_jpeg0_ctrl);

    return Error;
}

int rt_hw_jpeg_init(void)
{
    _SemaphoreJPEG = rt_sem_create("jpeg_sem", 1, RT_IPC_FLAG_PRIO);
    if (_SemaphoreJPEG == RT_NULL)
    {
        rt_kprintf("create jpeg decode semphr failed.\n");
        return RT_ERROR;
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_jpeg_init);
/*******************************************************************************************************************//**
 * @brief Decode callback function.
 * @param[in]  p_args
 * @retval     None
 **********************************************************************************************************************/
void decode_callback(jpeg_callback_args_t *p_args)
{
    rt_interrupt_enter();

    g_jpeg_status = p_args->status;

    rt_sem_release(_SemaphoreJPEG);

    rt_interrupt_leave();
}
#endif
