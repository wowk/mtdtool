#include "flash.h"
#include <stdio.h>
#include <stdlib.h>


void test_mtd4(void) {
    struct mtd_dev_t mtd;

    if(0 > mtd_open(&mtd, "/dev/mtd4", O_RDWR)) {
        perror("mtd_open");
        return;
    }

    char buffer[mtd.info.erasesize];
    
    for(int block = 0 ; block < 3 ; block ++) {
        if(mtd_is_bad_block(&mtd, block)) {
            printf("mtd4: block %d is a bad block\n", block);
            continue;
        }
        mtd_read_block(&mtd, block, buffer);
        mtd_write_block(&mtd, block, buffer);
    }

    mtd_close(&mtd);
}

void test_mtd5(void) {
    struct mtd_dev_t mtd;

    if(0 > mtd_open(&mtd, "/dev/mtd5", O_RDWR)) {
        perror("mtd_open");
        return;
    }

    char buffer[mtd.info.erasesize];
    
    for(int block = 0 ; block < 3 ; block ++) {
        if(mtd_is_bad_block(&mtd, block)) {
            printf("mtd5: block %d is a bad block\n", block);
            continue;
        }
        mtd_read_block(&mtd, block, buffer);
        mtd_write_block(&mtd, block, buffer);
    }

    mtd_close(&mtd);
}

void test_mtd8(void) {
    struct mtd_dev_t mtd;

    if(0 > mtd_open(&mtd, "/dev/mtd4", O_RDWR)) {
        perror("mtd_open");
        return;
    }

    char buffer[mtd.info.erasesize];
    
    for(int block = 1 ; block < 5 ; block ++) {
        if(mtd_is_bad_block(&mtd, block)) {
            printf("mtd8: block %d is a bad block\n", block);
            continue;
        }
        mtd_read_block(&mtd, block, buffer);
        mtd_write_block(&mtd, block, buffer);
    }

    mtd_close(&mtd);
}

int main(int argc, char *argv[]) {
    
    for(int i = atoi(argv[1]) ; i <= 100000 ; i ++) {
        printf("%dth read/erase/write\n", i);
        test_mtd4();
        test_mtd5();
        test_mtd8();
        sync();
    }

    return 0;
}
