#include "mtdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char * argv[]) {
    
    struct mtd_dev_t mtd;
    
    if(argc < 5) {
        printf("usage: setflag <mtdpath> <offset> <data> <size>\n");
        exit(0);
    }

    if(0 > mtd_open(&mtd, argv[1], O_RDWR)) {
        fprintf(stderr, "failed to open mtd %s: %m", argv[1]);
        exit(-errno);
    }

    struct mtd_info_user minfo;
    mtd_info(mtd.fd, &minfo);

    void * buffer = calloc(1, minfo.erasesize);
    if(buffer) {
        int block = mtd_next_valid_block(&mtd);
        if(block >= 0) {
            mtd_read_block(&mtd, block, buffer);
            mtd_erase_block(&mtd, block);
            int offset = atoi(argv[2]);
            int length = atoi(argv[4]);
            memcpy(buffer + offset, argv[3], length);
            mtd_write_block(&mtd, block, buffer);
        }
        free(buffer);
    }else{
        printf("failed to allocate memory: %m\n");
    }

    mtd_close(&mtd);

    return 0;
}
