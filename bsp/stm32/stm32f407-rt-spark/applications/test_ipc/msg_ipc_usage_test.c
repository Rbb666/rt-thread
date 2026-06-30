/*
 * msg_ipc_usage_test.c
 *
 * MSH command:
 *   msg_ipc_usage_test
 *
 * Shows the typical usage difference between mailbox and message queue.
 */
#include <rtthread.h>

#define MSG_IPC_MB_SIZE       4
#define MSG_IPC_MQ_MAX_MSG    32
#define MSG_IPC_MQ_MAX_COUNT  2

static rt_ubase_t msg_ipc_mb_pool[MSG_IPC_MB_SIZE];
static rt_uint8_t msg_ipc_mq_pool[RT_MQ_BUF_SIZE(MSG_IPC_MQ_MAX_MSG, MSG_IPC_MQ_MAX_COUNT)];

static void msg_ipc_case_mailbox(void)
{
    struct rt_mailbox mb;
    rt_ubase_t event;
    rt_ubase_t recv;

    rt_kprintf("\n[case1] mailbox: one word message\n");

    if (rt_mb_init(&mb, "mbo", msg_ipc_mb_pool, MSG_IPC_MB_SIZE, RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case1] mailbox init failed\n");
        return;
    }

    event = 0x1234;
    if (rt_mb_send(&mb, event) == RT_EOK)
    {
        rt_kprintf("[case1] send event 0x%x\n", event);
    }

    if (rt_mb_recv(&mb, &recv, RT_WAITING_NO) == RT_EOK)
    {
        rt_kprintf("[case1] recv event 0x%x\n", recv);
    }

    rt_kprintf("[case1] usage: event code, state value, or pointer\n");
    rt_mb_detach(&mb);
}

static void msg_ipc_case_messagequeue(void)
{
    struct rt_messagequeue mq;
    const char msg1[] = "TEMP=25";
    const char msg2[] = "HUMIDITY=60,PRESS=100";
    char recv[MSG_IPC_MQ_MAX_MSG];
    rt_ssize_t recv_len;

    rt_kprintf("\n[case2] message queue: variable length data with max size\n");

    if (rt_mq_init(&mq,
                   "mqu",
                   msg_ipc_mq_pool,
                   MSG_IPC_MQ_MAX_MSG,
                   sizeof(msg_ipc_mq_pool),
                   RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case2] message queue init failed\n");
        return;
    }

    rt_mq_send(&mq, msg1, sizeof(msg1));
    rt_mq_send(&mq, msg2, sizeof(msg2));

    rt_memset(recv, 0, sizeof(recv));
    recv_len = rt_mq_recv(&mq, recv, sizeof(recv), RT_WAITING_NO);
    if (recv_len > 0)
    {
        rt_kprintf("[case2] recv len %d, data \"%s\"\n", (int)recv_len, recv);
    }

    rt_memset(recv, 0, sizeof(recv));
    recv_len = rt_mq_recv(&mq, recv, sizeof(recv), RT_WAITING_NO);
    if (recv_len > 0)
    {
        rt_kprintf("[case2] recv len %d, data \"%s\"\n", (int)recv_len, recv);
    }

    rt_kprintf("[case2] usage: struct data, command packet, bounded variable length data\n");
    rt_mq_detach(&mq);
}

static void msg_ipc_usage_test(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("\n[msg_ipc_usage_test] start\n");

    msg_ipc_case_mailbox();
    msg_ipc_case_messagequeue();

    rt_kprintf("\n[summary]\n");
    rt_kprintf("  mailbox      : sends one rt_ubase_t value, usually pointer or event code\n");
    rt_kprintf("  message queue: copies data, supports variable length messages up to max size\n");
    rt_kprintf("[msg_ipc_usage_test] end\n");
}
MSH_CMD_EXPORT(msg_ipc_usage_test, show mailbox and message queue usage);
