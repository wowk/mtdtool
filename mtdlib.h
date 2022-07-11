#ifndef _FLASH_H_
#define _FLASH_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <mtd/mtd-user.h>

struct mtd_dev_t {
    int fd;
    struct mtd_info_user info;
};

#if defined(__cplusplus)
extern "C" {
#endif

size_t mtd_size(const struct mtd_dev_t *mtd);
size_t mtd_block_count(const struct mtd_dev_t* mtd);
int mtd_mark_as_bad_block(const struct mtd_dev_t *mtd, int block, int set);
int mtd_is_bad_block(const struct mtd_dev_t *mtd, int block);
int mtd_info(int fd, struct mtd_info_user* minfo);
int mtd_open(struct mtd_dev_t *mtd, const char *mtd_path, int mode);
void mtd_close(struct mtd_dev_t *mtd);
int mtd_erase_block(struct mtd_dev_t *mtd, size_t block);
int mtd_seek_block(struct mtd_dev_t *mtd, int block, int whence);
int mtd_current_block(const struct mtd_dev_t *mtd);
int mtd_eof(const struct mtd_dev_t *mtd, size_t block);
int mtd_next_valid_block(const struct mtd_dev_t *mtd);
ssize_t mtd_read_block(struct mtd_dev_t *mtd, int block, void* buffer);
ssize_t mtd_read_next_block(struct mtd_dev_t *mtd, void* buffer);
int mtd_verify_block(struct mtd_dev_t* mtd, int block, const void* data, void* buffer);
ssize_t mtd_write_block(struct mtd_dev_t *mtd, int block, const void* data);
ssize_t mtd_write_next_block(struct mtd_dev_t *mtd, const void* data);

int mtd_seek_page(struct mtd_dev_t *mtd, int block, int page, int whence);
ssize_t mtd_read_page(struct mtd_dev_t *mtd, int block, int page, void *buffer);
ssize_t mtd_write_page(struct mtd_dev_t *mtd, int block, int page, const void *data);

#if defined(__cplusplus)
}
#endif

#endif

