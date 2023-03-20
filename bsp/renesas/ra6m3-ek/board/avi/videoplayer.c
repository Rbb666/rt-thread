#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rthw.h>

#include "drv_jpeg.h"

#include "hal_data.h"
#include "pwm_audio.h"

#include <dfs_file.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "videoplayer.h"

#define USEING_AUDIO
/**
 * TODO: how to recognize each stream id
 */
#define T_vids _REV(0x30306463)
#define T_auds _REV(0x30317762)

struct q_rx_msg
{
    char *data;
    rt_size_t size;
};

rt_mq_t video_msg_mq = RT_NULL;
extern AVI_TypeDef AVI_file;

static uint32_t _REV(uint32_t value)
{
    return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
           (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

uint32_t read_frame(int fd, uint8_t *buffer, uint32_t length, uint32_t *fourcc)
{
    AVI_CHUNK_HEAD head;
    read(fd, &head, sizeof(AVI_CHUNK_HEAD));
    if (head.FourCC)
    {
        /* code */
    }
    *fourcc = head.FourCC;
    if (head.size % 2)
    {
        head.size++;
    }
    if (length < head.size)
    {
        rt_kprintf("frame size too large\n");
        return 0;
    }
    
    read(fd, buffer, head.size);
    return head.size;
}

void audio_init(void)
{
    pwm_audio_config_t pac;
    pac.duty_resolution    = 10;
    pac.gpio_num_left      = 1;
    pac.gpio_num_right     = -1;
    pac.ringbuf_len        = 1024 * 8;

    pwm_audio_init(&pac);
    
    /* set default volume:-16 -- +16*/ 
    pwm_audio_set_volume(-15);
}

int audio_volume_set(int argc, const char *argv[])
{
    if (argc != 2)
    {
        rt_kprintf("param not right!!\n");
        return RT_ERROR;
    }

    pwm_audio_set_volume(atoi(argv[1]));
    return RT_EOK;
}
MSH_CMD_EXPORT(audio_volume_set, audio_volume_set);

void video_play_thread(void *param)
{
    int ret;
    size_t BytesRD;
    uint32_t Strsize;
    uint32_t Strtype;
    uint8_t *pbuffer;
    uint32_t buffer_size = 30 * 1024;
    uint32_t alltime;
    uint32_t cur_time;

    pbuffer = rt_malloc(buffer_size);
    if (pbuffer == NULL)
    {
        rt_kprintf("cannot malloc memory for video palyer\n");
        return;
    }

    while (1)
    {
        int fd = -1;
        fd = open(param, O_WRONLY | O_CREAT);
        
        BytesRD = read(fd, pbuffer, 20480);    
        ret = AVI_Parser(pbuffer, BytesRD);
        if (0 > ret)
        {
            rt_kprintf("parse failed (%d)\n", ret);
            return;
        }
    
#ifdef USEING_AUDIO
        /* Audio Init */
        audio_init();
        pwm_audio_set_param(AVI_file.auds_sample_rate, AVI_file.auds_bits, AVI_file.auds_channels);
#endif
        uint16_t video_width = AVI_file.vids_width;
        uint16_t video_height = AVI_file.vids_height;
        rt_kprintf("video width:%d, video height:%d\n", video_width, video_height);

        // vido info
        alltime = (AVI_file.avi_hd.avih.us_per_frame / 1000) * AVI_file.avi_hd.avih.total_frames;
        alltime /= 1000; // s
        rt_kprintf("video total time:%02d:%02d:%02d\n", alltime / 3600, (alltime % 3600) / 60, alltime % 60);

        lseek(fd, AVI_file.movi_start, SEEK_SET);
        Strsize = read_frame(fd, pbuffer, buffer_size, &Strtype);
        BytesRD = Strsize + 8;
        
        while (1)
        {
            cur_time = ((float)BytesRD / AVI_file.movi_size) * alltime;

            if (BytesRD >= AVI_file.movi_size)
            {
                rt_kprintf("video played over\n");
                break;
            }
            if (Strtype == T_vids)
            {
                extern int JPEG_X_Draw(void *p, int x0, int y0);
                JPEG_X_Draw(pbuffer, 0, 0);
            }
            /* audio output */
            else if (Strtype == T_auds)
            {
                size_t cnt;
                pwm_audio_write((uint8_t *)pbuffer, Strsize, &cnt, 500);
            }
            else
            {
                rt_kprintf("unknow frame\n");
                break;
            }
            
            /* read frame */
            Strsize = read_frame(fd, pbuffer, buffer_size, &Strtype);
            BytesRD += Strsize + 8;
        }
        rt_kprintf("close video file\n");
        pwm_audio_deinit();
        close(fd);
    }
}
