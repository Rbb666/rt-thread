/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fal.h>
#include <board.h>
#include "gd32f4xx_fmc.h"

#define ONCHIP_FLASH_START_ADDR    ((uint32_t)0x08000000)
#define ONCHIP_FLASH_SIZE          (1024 * 1024)

static int onchip_flash_init(void)
{
    return 0;
}

static int onchip_flash_read(long offset, rt_uint8_t *buf, rt_size_t size)
{
    if ((offset < 0) || ((rt_size_t)offset + size > ONCHIP_FLASH_SIZE))
    {
        return -1;
    }

    rt_memcpy(buf, (const void *)(ONCHIP_FLASH_START_ADDR + offset), size);
    return size;
}

static uint32_t get_sector_by_addr(uint32_t addr)
{
    if (addr < 0x08004000)
        return CTL_SECTOR_NUMBER_0;
    if (addr < 0x08008000)
        return CTL_SECTOR_NUMBER_1;
    if (addr < 0x0800C000)
        return CTL_SECTOR_NUMBER_2;
    if (addr < 0x08010000)
        return CTL_SECTOR_NUMBER_3;
    if (addr < 0x08020000)
        return CTL_SECTOR_NUMBER_4;
    if (addr < 0x08040000)
        return CTL_SECTOR_NUMBER_5;
    if (addr < 0x08060000)
        return CTL_SECTOR_NUMBER_6;
    if (addr < 0x08080000)
        return CTL_SECTOR_NUMBER_7;
    if (addr < 0x080A0000)
        return CTL_SECTOR_NUMBER_8;
    if (addr < 0x080C0000)
        return CTL_SECTOR_NUMBER_9;
    if (addr < 0x080E0000)
        return CTL_SECTOR_NUMBER_10;

    return CTL_SECTOR_NUMBER_11;
}

static uint32_t get_sector_end_addr(uint32_t addr)
{
    if (addr < 0x08004000)
        return 0x08004000;
    if (addr < 0x08008000)
        return 0x08008000;
    if (addr < 0x0800C000)
        return 0x0800C000;
    if (addr < 0x08010000)
        return 0x08010000;
    if (addr < 0x08020000)
        return 0x08020000;
    if (addr < 0x08040000)
        return 0x08040000;
    if (addr < 0x08060000)
        return 0x08060000;
    if (addr < 0x08080000)
        return 0x08080000;
    if (addr < 0x080A0000)
        return 0x080A0000;
    if (addr < 0x080C0000)
        return 0x080C0000;
    if (addr < 0x080E0000)
        return 0x080E0000;

    return 0x08100000;
}

static int onchip_flash_erase(long offset, rt_size_t size)
{
    uint32_t addr;
    uint32_t end_addr;

    if ((offset < 0) || ((rt_size_t)offset + size > ONCHIP_FLASH_SIZE))
    {
        return -1;
    }

    addr = ONCHIP_FLASH_START_ADDR + offset;
    end_addr = addr + size;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);

    while (addr < end_addr)
    {
        if (fmc_sector_erase(get_sector_by_addr(addr)) != FMC_READY)
        {
            fmc_lock();
            return -1;
        }
        addr = get_sector_end_addr(addr);
    }

    fmc_lock();
    return size;
}

static int onchip_flash_write(long offset, const rt_uint8_t *buf, rt_size_t size)
{
    rt_size_t i;
    uint32_t addr;

    if ((offset < 0) || ((rt_size_t)offset + size > ONCHIP_FLASH_SIZE))
    {
        return -1;
    }

    addr = ONCHIP_FLASH_START_ADDR + offset;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);

    for (i = 0; i < size; i++)
    {
        if (fmc_byte_program(addr + i, buf[i]) != FMC_READY)
        {
            fmc_lock();
            return -1;
        }
    }

    fmc_lock();
    return size;
}

const struct fal_flash_dev gd32_onchip_flash =
{
    .name = "onchip_flash",
    .addr = ONCHIP_FLASH_START_ADDR,
    .len = ONCHIP_FLASH_SIZE,
    .blk_size = 16 * 1024,
    .ops =
    {
        .init = onchip_flash_init,
        .read = onchip_flash_read,
        .write = onchip_flash_write,
        .erase = onchip_flash_erase,
    },
    .write_gran = 8,
};
