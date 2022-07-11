#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <mtdlib.h>

size_t mtd_size(const struct mtd_dev_t *mtd) {
    return mtd->info.size;
}

size_t mtd_block_count(const struct mtd_dev_t *mtd) {
    return mtd->info.size/mtd->info.erasesize;
}

#define MEMSETGOODBLOCK		_IOW('M', 25, __kernel_loff_t)

int mtd_mark_as_bad_block(const struct mtd_dev_t *mtd, int block, int set) {
    int ret;
    loff_t start = block*mtd->info.erasesize;
    if(set){
        ret = ioctl(mtd->fd, MEMSETBADBLOCK, &start);
    }else{
        ret = ioctl(mtd->fd, MEMSETGOODBLOCK, &start);
    }
    if(ret < 0) {
        printf("failed to check if block %d(%llu) is a bad block: %m\n", block, start);
    }
    return ret;

}

int mtd_is_bad_block(const struct mtd_dev_t *mtd, int block) {
    loff_t start = block*mtd->info.erasesize;
    return ioctl(mtd->fd, MEMGETBADBLOCK, &start);
}

int mtd_info(int fd, struct mtd_info_user* minfo) {
    int ret = ioctl(fd, MEMGETINFO, minfo);
    if(ret < 0) {
        printf("failed to get mtd information: %m\n");
    }
    return ret;
}

int mtd_open(struct mtd_dev_t *mtd, const char *mtd_path, int mode) {
    mtd->fd = open(mtd_path, mode);
    if(0 > mtd->fd) {
        printf("failed to open <%s>: %m\n", mtd_path);
        return -errno;
    }

    if(0 > mtd_info(mtd->fd, &mtd->info)) {
        printf("failed to get info of <%s>: %m\n", mtd_path);
        close(mtd->fd);
        return -errno;
    }

    return 0;
}

void mtd_close(struct mtd_dev_t *mtd) {
    if(mtd->fd > 0) {
        close(mtd->fd);
        mtd->fd = -1;
    }
}

int mtd_erase_block(struct mtd_dev_t *mtd, size_t block){
    struct erase_info_user64 eiu;

    eiu.start   = block*mtd->info.erasesize;
    eiu.length  = mtd->info.erasesize;

    if (0 > ioctl(mtd->fd, MEMERASE64, &eiu)) {
        printf("failed to erase block %u: %m\n", (unsigned int)block);
        return -errno;
    }

    return 0;
}

int mtd_seek_block(struct mtd_dev_t *mtd, int block, int whence) {
    off_t offset = lseek(mtd->fd, block*mtd->info.erasesize, whence);
    if(offset == (off_t)-1) {
        printf("failed to lseek to block (%d,%d): %m\n", block, whence);
        return -1;
    }
    return (offset/mtd->info.erasesize);
}

int mtd_current_block(const struct mtd_dev_t *mtd) {
    off_t offset = lseek(mtd->fd, 0, SEEK_CUR);
    if(offset == (off_t)-1) {
        printf("failed to lseek current position: %m\n");
        return -1;
    }
    return offset/mtd->info.erasesize;
}

int mtd_eof(const struct mtd_dev_t *mtd, size_t block) {
    return (block*mtd->info.erasesize) >= mtd->info.size;
}

int mtd_next_valid_block(const struct mtd_dev_t *mtd) {
    int block = mtd_current_block(mtd);

    while(!mtd_eof(mtd, block) && mtd_is_bad_block(mtd, block)) {
        printf("block %d is bad, skip to next block\n", block);
        block ++;
    }
    
    return block;
}

ssize_t mtd_read_block(struct mtd_dev_t *mtd, int block, void* buffer){
    ssize_t ret;

    if(mtd_is_bad_block(mtd, block)) {
        return -EIO;
    }

    mtd_seek_block(mtd, block, SEEK_SET);
    while (0 > (ret=read(mtd->fd, buffer, mtd->info.erasesize)) && errno == EINTR);
    if (ret < 0) {
        printf("failed to read block: %m\n");
        return -errno;
    }

    return ret;
}

ssize_t mtd_read_next_block(struct mtd_dev_t *mtd, void* buffer){
    int block;

    block = mtd_next_valid_block(mtd);
    if(mtd_eof(mtd, block)) {
        printf("no valid block left\n");
        return -ENOMEM;
    }

    return mtd_read_block(mtd, block, buffer);
}

int mtd_verify_block(struct mtd_dev_t* mtd, int block, const void* data, void* buffer){
    int ret = 0;
    int old_block;
    
    old_block = mtd_current_block(mtd);

    mtd_seek_block(mtd, block, SEEK_SET);
    if (mtd->info.erasesize == mtd_read_block(mtd, block, buffer)) {
        if (memcmp(buffer, data, mtd->info.erasesize)) {
            printf("failed to verify data of block %d: not match\n", block);
            ret = -EIO;
        }
    }else{
        printf("failed to read back block %d to verify: %m\n", block);
        ret = -errno;
    }
    mtd_seek_block(mtd, old_block, SEEK_SET);

    return ret;
}

ssize_t mtd_write_block(struct mtd_dev_t *mtd, int block, const void* data) {
    int ret = 0;
    void* buffer;
    struct mtd_ecc_stats old, new;

    mtd_seek_block(mtd, block, SEEK_SET);
    ret = mtd_erase_block(mtd, block);
    if(0 > ret) {
        printf("failed to erase block %d", block);
        return ret;
    }
    
    ioctl(mtd->fd, ECCGETSTATS, &old);
    while(0 > (ret=write(mtd->fd, data, mtd->info.erasesize)) && errno == EINTR);
    if(0 > ret) {
        printf("failed to write block %d", block);
        return ret;
    }
    ioctl(mtd->fd, ECCGETSTATS, &new);

    if(ret != mtd->info.erasesize && new.failed != old.failed) {
        return -EIO;
    }
    
    buffer = malloc(mtd->info.erasesize);
    if(!buffer) {
        printf("failed to alloc memory for verify block %d: %m\n", block);
        return -errno;
    }

    ret = mtd_verify_block(mtd, block, data, buffer);
    if(0 > ret) {
        printf("failed to verify block %d\n", block);
    }
    free(buffer);

    return ret;
}

ssize_t mtd_write_next_block(struct mtd_dev_t *mtd, const void* data) {
    int block;
    
    while(1) {
        block = mtd_next_valid_block(mtd);
        if(mtd_eof(mtd, block)) {
            printf("failed to write block, no space left\n");
            return -ENOMEM;
        }else if(mtd->info.erasesize == mtd_write_block(mtd, block, data)) {
            break;
        }
    }

    return mtd->info.erasesize;
}

#if 0
int mtd_seek_page(struct mtd_dev_t *mtd, int block, int page, int whence) {
    assert(page >=0 && page < (mtd->info.erasesize/mtd->info.erasesize));
    return lseek(mtd->fd, block*mtd->info.erasesize+page*mtd->info.writesize, whence);
}

ssize_t mtd_read_page(struct mtd_dev_t *mtd, int block, int page, void *buffer) {
    mtd_seek_page(mtd, block, page, SEEK_SET);
    
}

ssize_t mtd_write_page(struct mtd_dev_t *mtd, int block, int page, const void *data) {

}
#endif
