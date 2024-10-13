#ifndef _MBR_H_
#define _MBR_H_

#include "common.h"

typedef struct
{
    uint8_t  status;
    uint8_t  start_head;
    uint16_t start_sector;
    uint8_t  part_type;
    uint8_t  end_head;
    uint16_t end_sector;
    uint32_t first_abs_sector;
    uint32_t sector_count;
} __attribute__((packed)) mbr_part_entry;

typedef struct
{
    uint8_t boot_code[440];
    int32_t unique_mbr_signature;
    int16_t unknown;
    mbr_part_entry partition_record[4]; 
    uint16_t signature;
} __attribute__((packed)) legacy_mbr;

#endif
