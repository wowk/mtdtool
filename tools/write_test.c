#include "mtdlib.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[]) {
    struct mtd_dev_t mtd;

    if(0 > mtd_open(&mtd, argv[1], O_RDWR)) {
        printf("failed to open mtd %s: %m\n");
        return -errno;
    }

    unsigned char buffer[mtd.info.erasesize];
    int block = atoi(argv[2]);//mtd_next_valid_block(&mtd);

    while(1) {
        if(0 > mtd_read_block(&mtd, block, buffer)) {
            printf("failed to read block: %m\n");
        }
        mtd_seek_block(&mtd, block, SEEK_SET);
        write(mtd.fd, buffer, mtd.info.writesize);
        fsync(mtd.fd);
    }

    mtd_close(&mtd);

    return 0;
}
