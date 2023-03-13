#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <rtthread.h>
#include "avifile.h"

#define PLAYER_SOUND_SIZE_DEFAULT     3    // 1-99
#define PLAYER_SONG_NAME_LEN_MAX     20

enum PLAYER_STATUS
{
    PLAYER_RUNNING,
    PLAYER_STOP
};

enum PLAYER_CMD
{
    PLAYER_CMD_PLAY,
    PLAYER_CMD_STOP,
    PLAYER_CMD_LAST,
    PLAYER_CMD_NEXT,
    PLAYER_CMD_SET_VOL,
    PLAYER_CMD_GET_VOL,
    PLAYER_CMD_GET_STATUS
};

struct player
{
    enum PLAYER_STATUS  status;
    uint8_t                  volume;
    uint8_t                  song_current;
    uint16_t                 song_time_pass;
    uint16_t                 song_time_all;
    
    rt_sem_t                 sem_play;
    rt_thread_t              play_thread;
};
typedef struct player *player_t;

#endif
