#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>
#include <mtdlib.h>
#include <flashmap.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


void usage(void) {
    printf("%s usage:\n", program_invocation_short_name);
    printf("\th: dump help information\n");
    printf("\tm: the mtd we want to operate on\n");
    printf("\ts: scan block\n");
    printf("\tS: scan all bad blocks\n");
    printf("\tV: scan all valid blocks\n");
    printf("\te: erase block\n");
    printf("\tE: [block] erase all blocks from [block]\n");
    printf("\tk: kill block\n");
    printf("\tr: [size] dump block\n");
    printf("\td: dump mtd information\n");
    printf("\tz: fill all blocks with zero\n");
    printf("\tf: <hex byte>: fill block with data\n");
    printf("\tF: <hex byte>: fill all blocks with data\n");
    printf("\tb: <block>: the block we want to operate on\n");
    printf("\tp: <page>: the page we want to operate on\n");
    exit(-1);
}




int main(int argc, char *argv[]) 
{
    int op;
    int mtd_num = -1;
    int page = -1;
    int block = -1;
    int blockCount;
    bool clear = false;
    bool mark = false;
    bool kill = false;
    bool scan = false;
    bool erase = false;
    bool fill = false;
    uint8_t fillData = 0;
    bool dump = false;
    bool scanAll = false;
    bool scanValid = false;
    bool scanAllValid = false;
    bool eraseAll = false;
    bool fillAll = false;
    bool dumpInfo = false;
    bool dumpAll = false;
    bool dumpFlashmap = false;
    int dumpSize = 0;
    int triggerSize = 1;
    bool triggerECC = false;
    bool triggerAllECC = false;
    struct flashmap_t *flashmap;

    while(-1 != (op=getopt(argc, argv, "m:dSsRrb:f:F:zeEhkMcp:Vvt:T:D"))) {
        switch(op) {
        case 'h':
            usage();
            break;
        case 'm':
            mtd_num = atoi(optarg);
            break;
        case 's':
            scan = true;
            break;
        case 'S':
            scanAll = true;
            break;
        case 'e':
            erase = true;
            break;
        case 'D':
            dumpFlashmap = true;
            break;
        case 'E':
            eraseAll = true;
            break;
        case 'F':
            fillAll = true;
            fillData = (uint8_t)strtoul(optarg, NULL, 16);
            break;
        case 'f':
            fill = true;
            fillData = (uint8_t)strtoul(optarg, NULL, 16);
            break;
        case 'z':
            fill = true;
            fillData = 0;
            break;
        case 'r':
            dump = true;
            dumpSize = atoi(optarg);
            break;
        case 'b':
            block = atoi(optarg);
            break;
        case 'p':
            page = atoi(optarg);
            break;
        case 'c':
            clear = true;
            break;
        case 'M':
            mark = true;
            break;
        case 'k':
            kill = true;
            break;
        case 'd':
            dumpInfo = true;
            break;
        case 'v':
            scanValid = true;
            break;
        case 'V':
            scanAllValid = true;
            break;
        case 't':
            triggerECC = true;
            triggerSize = atoi(optarg);
            break;
        case 'T':
            triggerAllECC = true;
            triggerSize = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if(dumpFlashmap) {
        flashmap_load(&flashmap);
        if(flashmap) {
            printf("%-10s\t%-10s\t%-s\n", "MTD", "Offset", "Size");
            for(int i = 0 ; i < flashmap->part_num ; i ++) {
                printf("mtd%-07d\t0x%.8X\t0x%X\n", i, 
                        flashmap->part[i].start, flashmap->part[i].size);
            }
            free(flashmap);
        }
        exit(0);
    }

    if(mtd_num == -1) {
        usage();
    }

    if(!(clear || erase || scan || mark || kill || fill || dump || triggerECC || scanAllValid ||
        dumpInfo || eraseAll || fillAll || scanAll || scanValid || triggerAllECC)) {
        usage();
    }

    char mtd_name[32] = "";
    snprintf(mtd_name, sizeof(mtd_name), "/dev/mtd%d", mtd_num);

    struct mtd_dev_t mtd;

    if(0 > mtd_open(&mtd, mtd_name, O_RDWR)) {
        printf("failed to open %s\n", mtd_name);
        return -errno;
    }

    if(clear || erase || scan || fill || dump || kill || mark || triggerECC) {
        if(block < 0 || block >= mtd_block_count(&mtd)) {
            printf("block %d invalid\n", block);
            mtd_close(&mtd);
            return -EINVAL;
        }
    }

    if(dumpInfo) {
        printf("total_size  : %u\n", mtd.info.size);
        printf("erase_size  : %u\n", mtd.info.erasesize);
        printf("write_size  : %u\n", mtd.info.writesize);
        printf("block_count : %u\n", mtd.info.size/mtd.info.erasesize);
    }
    
    if(triggerECC) {
        char *buffer;
        struct mtd_ecc_stats old, new;

        if(block < 0) {
            block = mtd_next_valid_block(&mtd);
            if(block < 0) {
                printf("no valid block\n");
                goto out;
            }
        }else if(mtd_is_bad_block(&mtd, block)) {
            printf("block %d is not a valid block\n", block);
            goto out;
        }

        if(!(buffer = (char*)malloc(mtd.info.erasesize))){
            printf("failed to allocate memory for ecc trigger\n");
        }else if(0 > mtd_read_block(&mtd, block, buffer)) {
            free(buffer);
            printf("failed to read block %d\n", block);
            goto out;
        }else {
            int done = 0;
            while(1) {
                ioctl(mtd.fd, ECCGETSTATS, &old);
                mtd_read_block(&mtd, block, buffer);
                mtd_seek_block(&mtd, block, SEEK_SET);
                write(mtd.fd, buffer, mtd.info.writesize*triggerSize);
                ioctl(mtd.fd, ECCGETSTATS, &new);
                if(old.failed != new.failed) {
                    if(done == 0){
                        printf("got uncorrectable ECC error, write another 99 times\n");
                    }
                    done ++;
                }
            }
            printf("trigger uncorrectable ECC error on block %d ... done\n", block);
            free(buffer);
        }
    }

    if(triggerAllECC) {
        char *buffer;
        struct mtd_ecc_stats old, new;

        if(block < 0) {
            block = mtd_next_valid_block(&mtd);
            if(block < 0) {
                printf("no valid block\n");
                goto out;
            }
        }
        
        if(!(buffer = (char*)malloc(mtd.info.erasesize))){
            printf("failed to allocate memory for ecc trigger\n");
            goto out;
        }

        blockCount = mtd_block_count(&mtd) - block;
        for(; block < blockCount ; block ++) {
            if(mtd_is_bad_block(&mtd, block)) {
                printf("block %d is not a valid block\n", block);
                continue;
            }if(0 > mtd_read_block(&mtd, block, buffer)) {
                printf("failed to read block %d\n, skip over", block);
                continue;
            }else {
                int done = 0;
                while(done < 100) {
                    ioctl(mtd.fd, ECCGETSTATS, &old);
                    mtd_read_block(&mtd, block, buffer);
                    mtd_seek_block(&mtd, block, SEEK_SET);
                    write(mtd.fd, buffer, mtd.info.writesize*triggerSize);
                    ioctl(mtd.fd, ECCGETSTATS, &new);
                    if(old.failed != new.failed) {
                        if(done == 0){
                            printf("got uncorrectable ECC error on block %d, write another 99 times\n", block);
                        }
                        done ++;
                    }
                }
                printf("trigger uncorrectable ECC error on block %d ... done\n", block);
            }
        }
    }

    if(scan) {
        if(mtd_is_bad_block(&mtd, block)) {
            printf("block %d is bad\n", block);
        }else{
            printf("block %d is good\n", block);
        }
    }
    
    if(scanValid) {
        if(block < 0) {
            int vblock = mtd_next_valid_block(&mtd);
            if(vblock >= 0) {
                printf("first valid block is %d", vblock);
            }else {
                printf("no valid block exist\n");
            }
        }else{
            if(block >= mtd_block_count(&mtd)){
                printf("block %d out of range\n", block);
            }else if(!mtd_is_bad_block(&mtd, block)){
                printf("block %d is valid\n", block);
            }else{
                printf("block %d is bad block\n", block);
            }
        }
    }

    if(kill) {
        if(mtd_is_bad_block(&mtd, block)) {
            printf("block is already bad\n");
        }else {
#if 0
            char buff[mtd.info.erasesize];
            for(int i = 0 ; ; i ++) {
                if(mtd_is_bad_block(&mtd, block)) {
                    break;
                }
                mtd_erase_block(&mtd, block);
                mtd_write_page(&mtd, block, buff);
                if((i %100) == 0) {
                    printf("\rErase %dth", i);
                    fflush(stdout);
                }
                fsync(mtd.fd);
            }
#else
            mtd_mark_as_bad_block(&mtd, block, 1);
#endif

            printf("Block %d killed\n", block);
        }
    }

    if(mark) {
         if(mtd_is_bad_block(&mtd, block)) {
            printf("block is already bad\n");
        }else {
            mtd_mark_as_bad_block(&mtd, block, 1);
            if(!mtd_is_bad_block(&mtd, block)) {
                printf("failed to mark block %d as bad block: %m\n", block);
            }else{
                printf("mark block %d as bad block successfully\n", block);
            }
        }
    }

    if(clear) {
        if(!mtd_is_bad_block(&mtd, block)) {
            printf("block is already good\n");
        }else {
            mtd_mark_as_bad_block(&mtd, block, 0);
            if(mtd_is_bad_block(&mtd, block)) {
                printf("failed to mark block %d as good block: %m\n", block);
            }else{
                printf("mark block %d as good block successfully\n", block);
            }
        }
    }
    
    if(erase) {
        if(0 > mtd_erase_block(&mtd, block)) {
            printf("failed to erase block %d\n", block);
        }else{
            printf("erase block %d done\n", block);
        }
    }

    if(fill) {
        void* data = malloc(mtd.info.erasesize);
        if(!data) {
            printf("failed to allocate memory: %m\n");
            printf("failed to fill block %d with data: 0x%.8x\n", block, fillData);
            return -ENOMEM;
        }
        memset(data, fillData, mtd.info.erasesize);
        if(0 > mtd_write_block(&mtd, block, data)) {
            printf("failed to fill block %d with data: 0x%.8x\n", block, fillData);
        }
        free(data);
    }
    
    if(dump) {
        void* buffer = malloc(mtd.info.erasesize);
        if(!buffer) {
            printf("failed to allocate memory: %m\n");
            printf("failed to read block %d's data: %m\n", block);
            return -ENOMEM;
        }

        if(mtd_is_bad_block(&mtd, block)) {
            printf("block %d is bad block\n", block);
        }
        if(0 > mtd_read_block(&mtd, block, buffer)) {
            printf("failed to read block %d's data: %m\n", block);
        }else{
            for(int i = 0 ; i < mtd.info.erasesize ; i ++) {
                if(i && (i%16) == 0) {
                    printf("\n");
                }
                printf("%.2x ", ((uint8_t*)buffer)[i]);
            }
            printf("\n");
        }
        free(buffer);
    }

    if(scanAll || scanAllValid) {
        printf("total_size  : %u\n", mtd.info.size);
        printf("erase_size  : %u\n", mtd.info.erasesize);
        printf("write_size  : %u\n", mtd.info.writesize);
        printf("block_count : %u\n", mtd_block_count(&mtd));
        for(int i = 0 ; i < mtd_block_count(&mtd) ; i ++) {
            printf("\r\bscan block %d ...", i);
            fflush(stdout);
            if(mtd_is_bad_block(&mtd, i)) {
                if(scanAll){
                    printf("\nblock %d is bad block\n", i);
                }
            }else if(scanAllValid) {
                printf("\nblock %d is valid\n", i);
            }
        }
        printf("\ndone!\n");
    }
    
    if(eraseAll) {
        int i = (block > 0) ? block : 0;
        for(; i < mtd_block_count(&mtd) ; i ++) {
            printf("\r\berase block %d ...", i);
            fflush(stdout);
            if(0 > mtd_erase_block(&mtd, i)) {
                printf("\nfailed to erase block %d\n", i);
            }
        }
        printf("done\n");
    }

    if(fillAll) {
        void* data = malloc(mtd.info.erasesize);
        if(!data) {
            printf("failed to allocate memory: %m\n");
            printf("failed to fill mtd%d with data: 0x%.8x\n", mtd_num, fillData);
            return -ENOMEM;
        }
        memset(data, fillData, mtd.info.erasesize);

        for(int i = 0 ; i < mtd_block_count(&mtd) ; i ++) {
            if(0 > mtd_write_block(&mtd, block, data)) {
                printf("failed to fill block %d with data: 0x%.8x\n", i, fillData);
            }
        }
        free(data);
    }

    if(dumpAll) {
        void* buffer = malloc(mtd.info.erasesize);
        if(!buffer) {
            printf("failed to allocate memory: %m\n");
            printf("failed to read block mtd%d's data: %m\n", mtd_num);
            return -ENOMEM;
        }
        for(int i = 0 ; i < mtd_block_count(&mtd) ; i ++) {
            if(0 > mtd_read_block(&mtd, i, buffer)) {
                printf("failed to read block %d's data: %m\n", i);
            }else{
                for(int i = 0 ; i < mtd.info.erasesize ; i ++) {
                    if(i && (i%16) == 0) {
                        printf("\n");
                    }
                    printf("%.2x ", ((uint8_t*)buffer)[i]);
                }
                printf("\n");
            }
        }
        free(buffer);
    }
out:
    mtd_close(&mtd);
    
    sync();

    return 0;
}
