#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gpt.h"

static inline uint32_t __efi_crc32(const void *buf, int len, uint32_t seed)
{
    int i;
    register uint32_t crc32val;
    const unsigned char *s = buf;
    crc32val = seed;
    for (i = 0; i < len; i++)
    {
        crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val;
}

static inline uint32_t crc32(const void *buf, int len)
{
    return (__efi_crc32(buf, len, ~0L) ^ ~0L);
}

// Guid 使用的是混合字节序，
// 在转换为字节流时要考虑大小端的问题，
// 前三个部分是大端后两个部分是小端。
static inline PSTR uuid_to_str(uint8_t *data)
{
    PSTR buf = (PSTR)malloc(37);
    PCSTR hex = "0123456789ABCDEF";
    PSTR p = buf;

    uint32_t num = *((uint32_t *)data);
    p += sprintf(p, "%08X-", num);
    data += 4;

    uint16_t num2 = *((uint16_t *)data);
    p += sprintf(p, "%04hX-", num2);
    data += 2;

    num2 = *((uint16_t *)data);
    p += sprintf(p, "%04hX-", num2);
    data += 2;

    int i;
    for (i = 0; i < 8; i++)
    {
        if (i == 2)
            *p++ = '-';
        *p++ = hex[data[i] >> 4];
        *p++ = hex[data[i] & 0xF];
    }

    buf[36] = '\0';
    return buf;
}

static inline wchar_t *utf16_to_wchar(uint16_t *str, int len)
{
    int i = 0;
    wchar_t *buf = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
    for (; i < len; i++)
        buf[i] = (wchar_t)str[i];
    buf[len] = L'\0';
    return buf;
}

int measure_block_size(int fd, int isAlt)
{
    int i = 1;
    int offset;
    int size = 512;
    int whence = isAlt ? SEEK_END : SEEK_SET;
    uint64_t signature;
    for (; i < 9; i++)
    {
        offset = i * size;
        if (lseek(fd, isAlt ? (-1 * offset) : offset, whence) == -1)
        {
            perror("Can't lseek");
            return -1;
        }
        signature = 0;
        if (read(fd, &signature, 8) == -1)
        {
            perror("read file error");
            return -1;
        }
        if (signature == GPT_HEADER_SIGNATURE)
            return offset;
    }
    return 0;
}

int print_gpt_header(int fd, gpt_header *gpt)
{
    memset(gpt, 0, sizeof(gpt_header));
    if (read(fd, gpt, sizeof(gpt_header)) == -1)
    {
        perror("read file error");
        return -1;
    }

    char sz[9];
    memcpy(sz, &gpt->signature, 8);
    sz[8] = '\0';
    printf("GPT:\n");
    printf("签名                    =   %s\n", sz);
    printf("版本号                  =   0x%08X\n", gpt->version);
    printf("GPT表头大小             =   %d\n", gpt->header_size);
    printf("CRC-32校验              =   0x%08X\n", gpt->crc_header);
    printf("表头扇区号              =   0x%08llX\n", gpt->lba_current);
    printf("备份表头扇区号          =   0x%08llX\n", gpt->lba_backup);
    printf("GPT分区起始扇区号       =   0x%08llX\n", gpt->lba_first_usable);
    printf("GPT分区结束扇区号       =   0x%08llX\n", gpt->lba_last_usable);
    PSTR tmp = uuid_to_str(gpt->disk_guid);
    printf("磁盘GUID                =   %s\n", tmp);
    free(tmp);
    printf("分区表起始扇区号        =   0x%08llX\n", gpt->lba_entry_start);
    printf("分区表总项数            =   0x%0X\n", gpt->entry_count);
    printf("单个分区表占用字节数    =   0x%0X\n", gpt->entry_size);
    printf("分区表的CRC校验         =   0x%08X\n", gpt->crc_entries);
    double size_byte = (gpt->lba_backup + 1) * logical_blk_size;
    double size_MB = size_byte / 1000.0 / 1000.0;
    double size_GB = size_MB / 1000.0;
    printf("硬盘总大小              =   %.0lf Bytes / %lf MB / %lf GB\n", size_byte, size_MB, size_GB);
    uint32_t oldcrc = gpt->crc_header;
    gpt->crc_header = 0;
    if (crc32(gpt, gpt->header_size) != oldcrc)
        fputs("Bad crc_header.\n", stderr);
    gpt->crc_header = oldcrc;
    putchar('\n');
    return 0;
}

int print_gpt_partitions(int fd, const gpt_header *gpt)
{
    int entries_size = gpt->entry_count * gpt->entry_size;
    PSTR buf = (PSTR)malloc(entries_size);
    if (read(fd, buf, entries_size) == -1)
    {
        perror("read file error");
        return -1;
    }

    wchar_t *gpt_entry_name;
    partition_entry *gentry = (partition_entry *)buf;
    partition_entry gentry_empty;
    memset(&gentry_empty, 0, sizeof(partition_entry));

    PSTR tmp;

    for (int i = 0; i < gpt->entry_count; i++)
    {
        if (!memcmp(gentry, &gentry_empty, sizeof(partition_entry)))
            continue;
        gpt_entry_name = utf16_to_wchar(gentry->name, 36);
        tmp = uuid_to_str(gentry->type_guid);
        printf("第 %d 个分区表:\n", i + 1);
        printf("标明分区类型的GUID  =   %s\n", tmp);
        free(tmp);
        tmp = uuid_to_str(gentry->unique_guid);
        printf("分区特有的GUID      =   %s\n", tmp);
        free(tmp);
        printf("该分区起始扇区号    =   0x%08llX\n", gentry->lba_first);
        printf("该分区起始扇区号    =   0x%08llX\n", gentry->lba_last);
        printf("分区的属性标志      =   0x%08llX\n", gentry->attribute);
        printf("该分区名            =   %ls\n", gpt_entry_name);
        double size_byte = (gentry->lba_last - gentry->lba_first + 1) * logical_blk_size;
        double size_MB = size_byte / 1000.0 / 1000.0;
        double size_GB = size_MB / 1000.0;
        printf("该分区大小          =   %.0lf Bytes / %lf MB / %lf GB\n", size_byte, size_MB, size_GB);
        printf("\n");
        free(gpt_entry_name);
        gentry++;
    }

    if (crc32(buf, entries_size) != gpt->crc_entries)
        fputs("Bad crc_entries.\n", stderr);

    free(buf);
    putchar('\n');
    return 0;
}
