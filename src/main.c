#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gpt.h"


int main(int argc, PCSTR argv[])
{
    int exitCode = EXIT_SUCCESS;
    PCSTR filename;
    int isAlternate = 0;
    int res = 0;
    if (argc < 2)
    {
        fprintf(stderr, "%s: Missing argument.\n", argv[0]);
        fprintf(stderr, "Usage1: Show primary GPT header and entries");
        fprintf(stderr, "          %s /dev/disk0", argv[0]);
        fprintf(stderr, "Usage2: Show secondary GPT header and entries, (but seems to be deprecated)");
        fprintf(stderr, "          %s /dev/disk0 alternate", argv[0]);
        return EXIT_FAILURE;
    }

    filename = argv[1];
    if (argc > 2)
        isAlternate = 1;

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Can't open device");
        return EXIT_FAILURE;
    }

    logical_blk_size = measure_block_size(fd, isAlternate);
    if (logical_blk_size < 1)
    {
        if (!logical_blk_size)
            fputs("Unknown logical block size.\n", stderr);
        exitCode = EXIT_FAILURE;
        goto END;
    }
    printf("逻辑块大小: %d bytes\n\n", logical_blk_size);
 
    if (isAlternate)
    {
        if (lseek(fd, -1 * logical_blk_size, SEEK_END) == -1)
        {
            perror("Can't lseek");
            exitCode = EXIT_FAILURE;
            goto END;
        }
    }
    else
    {
        if (lseek(fd, 0, SEEK_SET) == -1)
        {
            perror("Can't lseek");
            exitCode = EXIT_FAILURE;
            goto END;
        }
        res = print_legacy_mbr(fd);
        if (res != 0)
        {
            exitCode = EXIT_FAILURE;
            goto END;
        }

        if (lseek(fd, logical_blk_size, SEEK_SET) == -1)
        {
            perror("Can't lseek");
            exitCode = EXIT_FAILURE;
            goto END;
        }
    }

    gpt_header header;
    res = print_gpt_header(fd, &header);
    if (res != 0)
    {
        exitCode = EXIT_FAILURE;
        goto END;
    }

    if (isAlternate)
    {
        if (lseek(fd, -1 * (header.entry_count * header.entry_size + logical_blk_size), SEEK_END) == -1)
        {
            perror("Can't lseek");
            exitCode = EXIT_FAILURE;
            goto END;
        }
    }
    else if (lseek(fd, logical_blk_size * header.lba_entry_start, SEEK_SET) == -1)
    {
        perror("Can't lseek");
        exitCode = EXIT_FAILURE;
        goto END;
    }

    res = print_gpt_partitions(fd, &header);
    if (res != 0) {
        exitCode = EXIT_FAILURE;
        goto END;
    }

END:
    close(fd);
    return exitCode;
}