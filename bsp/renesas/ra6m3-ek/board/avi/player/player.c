#include <rthw.h>

#include "player.h"
#include "avifile.h"
#include "pwm_audio.h"

#include <dfs_file.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#define DBG_TAG    "player"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#define T_vids _REV(0x30306463)
#define T_auds _REV(0x30317762)

struct avi_file_info
{
    uint32_t Strsize;
    uint32_t Strtype;
    size_t BytesRD;
    uint32_t cur_time;
    uint32_t alltime;
}avi_file;

#define v_pbuffer_size 35 * 1024
static uint8_t *v_pbuffer;

extern AVI_TypeDef AVI_file;

static uint32_t _REV(uint32_t value)
{
    return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
           (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

void readdir_sample(player_t player)
{
    DIR *dirp;
    struct dirent *d;

    dirp = opendir("/res");
    if (dirp == RT_NULL)
    {
        LOG_E("open directory error!\n");
    }
    else
    {
        while ((d = readdir(dirp)) != RT_NULL)
        {
            LOG_I("found %s", d->d_name);
            if (rt_strstr(d->d_name, ".avi"))
            {
                char file_name[30];
                rt_sprintf(file_name, "/res/%s", d->d_name);
                player->video_list[player->song_num] = rt_strdup(file_name);
                LOG_E("video_list[%d]:%s", player->song_num, player->video_list[player->song_num]);
                player->song_num ++;
            }
        }
        closedir(dirp);
    }
}

//
struct player v_player;

int player_init_test(void)
{
    /* read filesystem */
    player_start(&v_player);
    return 0;
}
INIT_APP_EXPORT(player_init_test);
//

static void audio_init(player_t player)
{
    pwm_audio_config_t pac;
    pac.duty_resolution    = 10;
    pac.gpio_num_left      = 1;
    pac.gpio_num_right     = -1;
    pac.ringbuf_len        = 1024 * 8;

    pwm_audio_init(&pac);
    
    /* set default volume:-16 -- +16*/ 
    pwm_audio_set_volume(player->volume);
}

uint32_t read_video_frame(int fd, uint8_t *buffer, uint32_t length, uint32_t *fourcc)
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

int video_start_parser(player_t player, int fd, char *filename)
{
    int ret;
    LOG_I("filename:%s", filename);

    fd = open(filename, O_WRONLY | O_CREAT);
    
    avi_file.BytesRD = read(fd, v_pbuffer, 20480);    
    ret = AVI_Parser(v_pbuffer, avi_file.BytesRD);
    if (0 > ret)
    {
        LOG_E("parse failed (%d)\n", ret);
        return RT_ERROR;
    }

    /* Audio Init */
    audio_init(player);
    pwm_audio_set_param(AVI_file.auds_sample_rate, AVI_file.auds_bits, AVI_file.auds_channels);

    lseek(fd, AVI_file.movi_start, SEEK_SET);
    avi_file.Strsize = read_video_frame(fd, v_pbuffer, v_pbuffer_size, &avi_file.Strtype);
    avi_file.BytesRD = avi_file.Strsize + 8;

    return fd;
}

int player_show(player_t player)
{
    uint8_t i;
    uint16_t percent;

    rt_kprintf("*********** video Player ***********\n");

    /* 打印歌单 */
    for (i = 0; i < player->song_num; i++)
    {
        rt_kprintf("%02d. %s\n", i + 1, (char *)player->video_list[i]);
    }

    /* 打印当前播放状态 */
    if (PLAYER_RUNNING == player->status)
    {
        rt_kprintf("<---  正在播放:");
    }
    else
    {
        rt_kprintf("<---  暂停播放:");
    }

    /* 打印当前歌曲 */
    rt_kprintf("%s", (char *)player->video_list[player->song_current - 1]);
    rt_kprintf("--->\n");

    /* 打印播放进度 */
    percent = player->song_time_pass * 100 / player->song_time_all;
    rt_kprintf("播放进度：%02d%%  音量大小：%02d%%\n", percent, player->volume);

    rt_kprintf("***********************************\n");

    return 0;
}

int player_init(player_t player)
{
    rt_uint32_t level;

    if (player->status != PLAYER_RUNNING)
    {
        level = rt_hw_interrupt_disable();

        player->status = PLAYER_READY;

        rt_hw_interrupt_enable(level);
        
        rt_sem_release(player->sem_play);
    }

    return 0;
}

int player_play(player_t player)
{
    rt_uint32_t level;

    if (player->status != PLAYER_RUNNING)
    {
        level = rt_hw_interrupt_disable();

        player->status = PLAYER_RUNNING;

        rt_hw_interrupt_enable(level);

        rt_sem_release(player->sem_play);
    }

    return 0;
}

int player_stop(player_t player)
{
    rt_uint32_t level;

    if (player->status == PLAYER_RUNNING)
    {
        level = rt_hw_interrupt_disable();

        player->status = PLAYER_STOP;

        rt_hw_interrupt_enable(level);
    }

    return 0;
}

int player_delete(player_t player)
{
    rt_uint32_t level;

    if (player->status == PLAYER_RUNNING)
    {
        level = rt_hw_interrupt_disable();

        player->status = PLAYER_DELETE;

        rt_hw_interrupt_enable(level);
    }

    return 0;
}

int player_last(player_t player)
{
    rt_uint32_t level;

    level = rt_hw_interrupt_disable();

    if (player->song_current > 1)
    {
        player->song_current --;
    }
    else
    {
        player->song_current = player->song_num;
    }

    player->song_time_pass = 0;

    rt_hw_interrupt_enable(level);

    player->status = PLAYER_LAST;

    level = rt_hw_interrupt_disable();

//    player->song_time_all = len;

    rt_hw_interrupt_enable(level);

    return 0;
}

int player_next(player_t player)
{
    rt_uint32_t level;

    level = rt_hw_interrupt_disable();

    if (player->song_current < player->song_num)
    {
        player->song_current ++;
    }
    else
    {
        player->song_current = 1;
    }

    player->song_time_pass = 0;

    rt_hw_interrupt_enable(level);

    player->status = PLAYER_NEXT;

    level = rt_hw_interrupt_disable();

//    player->song_time_all = len;

    rt_hw_interrupt_enable(level);

    return 0;
}

uint8_t add_video_player(player_t player)
{
    uint8_t video_index = 0;

    if (player->song_num == 0)
    {
        player->video_list[player->song_current] = rt_strdup(player->video_name);
        LOG_I("添加第一个视频：%s", player->video_list[player->song_current]);
        player->song_current ++;
        player->song_num++;
    }
    else
    {
        rt_bool_t flag = RT_FALSE;
        /* 遍历播放列表，若不存在则添加进去 */
        for (int index = 0; index < player->song_num; index++)
        {
            char *video_name = player->video_list[index];
            /* 不等于则查找下一个 */
            if (rt_strcmp(player->video_name, video_name))
            {
                flag = RT_TRUE;
            }
            else
            {
                flag = RT_FALSE;
                video_index = index;
                LOG_I("查找到名称对应的字段，位置：%d", video_index);
                break;
            }
        }
        if (flag)
        {
            player->video_list[player->song_current] = rt_strdup(player->video_name);
            LOG_I("添加 [%s] 到视频列表", player->video_list[player->song_current]);
            player->song_current ++;
            player->song_num ++;
            /* 释放当前资源，播放新视频 */
            player_delete(player);
        }
    }
    /* 展示下列表 */
    player_show(player);
    
    return video_index;
}


int player_control(player_t player, int cmd, void *arg)
{
    rt_uint32_t level;

    switch (cmd)
    {
    case PLAYER_CMD_INIT:
        LOG_I("接收视频文件:%s len:%d", (char *)arg, rt_strlen(arg));
        rt_memset(player->video_name, 0x00, rt_strlen(arg));
        rt_strcpy(player->video_name, arg);
        player->song_current = add_video_player(player) + 1;
        player_init(player);
        break;
    case PLAYER_CMD_PLAY:
        player_play(player);
        break;
    case PLAYER_CMD_STOP:
        player_stop(player);
        break;
    case PLAYER_CMD_LAST:
        player_last(player);
        break;
    case PLAYER_CMD_NEXT:
        player_next(player);
        break;
    case PLAYER_CMD_SET_VOL:
        level = rt_hw_interrupt_disable();
        player->volume = *(int16_t*)arg;
        rt_hw_interrupt_enable(level);
        pwm_audio_set_volume(player->volume);
        break;
    case PLAYER_CMD_GET_VOL:
        *(uint8_t *)arg = player->volume;
        break;
    case PLAYER_CMD_GET_STATUS:
        *(uint8_t *)arg = player->status;
        break;
    }
    return 0;
}

int video_play_cmd()
{
    player_play(&v_player);
    return RT_EOK;
}
MSH_CMD_EXPORT(video_play_cmd, play video)

int video_stop_cmd()
{
    player_stop(&v_player);
    return RT_EOK;
}
MSH_CMD_EXPORT(video_stop_cmd, stop play video)

static void player_entry(void *parameter)
{
    player_t player = (player_t)parameter;

    int fd = -1;
    while (1)
    {
        if (player->status == PLAYER_READY)
        {
            fd = video_start_parser(player, fd, (char *)player->video_list[player->song_current - 1]);
            LOG_I("视频：%s准备解码\n", (char *)player->video_list[player->song_current - 1]);
            
            player->status = PLAYER_RUNNING;
        }
        if (player->status == PLAYER_RUNNING)
        {
            avi_file.cur_time = ((float)avi_file.BytesRD / AVI_file.movi_size) * avi_file.alltime;

            if (avi_file.Strtype == T_vids)
            {
                extern int JPEG_X_Draw(void *p, int x0, int y0);
                JPEG_X_Draw(v_pbuffer, 0, 0);
            }
            /* audio output */
            else if (avi_file.Strtype == T_auds)
            {
                size_t cnt;
                pwm_audio_write((uint8_t *)v_pbuffer, avi_file.Strsize, &cnt, 500);
            }
            else
            {
                LOG_E("unknow frame\n");
                break;
            }
            
            /* read frame */
            avi_file.Strsize = read_video_frame(fd, v_pbuffer, v_pbuffer_size, &avi_file.Strtype);
            avi_file.BytesRD += avi_file.Strsize + 8;
            
            /* 如果播放时间到了，暂停该视频 */
            if (avi_file.BytesRD >= AVI_file.movi_size)
            {
                player_next(player);
                player_show(player);
            }
        }
        if (player->status == PLAYER_STOP)
        {
            LOG_I("暂停播放");

            pwm_audio_stop();

            rt_sem_take(player->sem_play, RT_WAITING_FOREVER);

            pwm_audio_start();
        }
        if (player->status == PLAYER_DELETE)
        {
            player->status = PLAYER_IDLE;

            LOG_I("释放%s资源", player->video_list[player->song_current - 1]);
            pwm_audio_deinit();
            close(fd);
        }
        if (player->status == PLAYER_LAST)
        {
            LOG_I("释放%s资源", player->video_list[player->song_current - 1]);
            pwm_audio_deinit();
            close(fd);
            player->status = PLAYER_READY;
        }
        if (player->status == PLAYER_NEXT)
        {
            LOG_I("释放%s资源", player->video_list[player->song_current - 1]);
            pwm_audio_deinit();
            close(fd);
            player->status = PLAYER_READY;
        }
        if (player->status == PLAYER_IDLE)
        {
            rt_sem_take(player->sem_play, RT_WAITING_FOREVER);
        }
    }
}

int player_start(player_t player)
{
    static rt_uint8_t inited = 0;

    if (inited == 1)
    {
        return -RT_ERROR;
    }

    v_pbuffer = rt_malloc(v_pbuffer_size);
    RT_ASSERT(v_pbuffer != NULL)

    /* read filesystem */
    readdir_sample(player);

    player->status = PLAYER_IDLE;
    player->volume = PLAYER_SOUND_SIZE_DEFAULT;
    player->song_current = 1;
    player->song_time_pass = 0;

    player->sem_play = rt_sem_create("sem_play", 0, RT_IPC_FLAG_FIFO);
    if (player->sem_play == RT_NULL)
    {
        return -RT_ERROR;
    }

    player->play_thread = rt_thread_create("player",
                                           player_entry, player,
                                           2 * 1024, 18, 20);
    if (player->play_thread != RT_NULL)
    {
        rt_thread_startup(player->play_thread);
    }
    else
    {
        rt_sem_delete(player->sem_play);
        return -RT_ERROR;
    }
    inited = 1;

    return 0;
}
