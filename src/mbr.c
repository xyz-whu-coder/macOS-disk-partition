#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mbr.h"

int print_legacy_mbr(int fd)
{
    legacy_mbr mbr;
    memset(&mbr, 0, sizeof(legacy_mbr));
    if (read(fd, &mbr, sizeof(legacy_mbr)) == -1)
    {
        perror("read file error");
        return -1;
    }

    mbr_part_entry empty;
    memset(&empty, 0, sizeof(mbr_part_entry));
    mbr_part_entry *p;
    int i = 0;
    printf("MBR:\n");
    printf("引导程序:\n");
    for (i = 0; i < 440; i++)
    {
        printf("%02X ", mbr.boot_code[i]);
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
    printf("  \n磁盘签名            =   0x%08X", mbr.unique_mbr_signature);
    printf("  \n未知                =   0x%04X\n", mbr.unknown);
    for (i = 0; i < 4; i++)
    {
        p = &mbr.partition_record[i];
        if (!memcmp(p, &empty, sizeof(mbr_part_entry)))
            continue;

        printf("活动状态            =   0x%02hhX\n", p->status);
        printf("开始磁头            =   0x%02hhX\n", p->start_head);
        printf("开始扇区和柱面      =   0x%04hX\n", p->start_sector);
        printf("分区类型            =   0x%02hhX\n", p->part_type);
        printf("结束磁头            =   0x%02hhX\n", p->end_head);
        printf("结束扇区和柱面      =   0x%04hX\n", p->end_sector);
        printf("这个分区前的扇区数  =   0x%08X\n", p->first_abs_sector);
        printf("这个分区的扇区数    =   0x%08X\n", p->sector_count);
    }

        printf("结束标志            =   0x%hX\n", mbr.signature);
    putchar('\n');
    return 0;
}

