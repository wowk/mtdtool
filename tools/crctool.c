#include <flash.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define FW_KFSINFO_OFFSET   0x800
#define BOOT_KFSINFO_OFFSET 0x1000

enum {
    KERNEL_SIZE_INDEX = 1,
    ROOTFS_SIZE_INDEX = 2,
    CFERAM_SIZE_INDEX = 1,
    KERNEL_CRC_INDEX  = 0,
    ROOTFS_CRC_INDEX  = 1,
    CFERAM_CRC_INDEX  = 0,
    FW_CRC_EN_INDEX   = 2,
};

struct crc_section_t {
    uint8_t magic[32];
    uint32_t size[8];
    uint8_t version[32];
    uint32_t cksum[8];
} __attribute__((packed));


int main(int argc, char *argv[])
{
    struct mtd_dev_t mdev;

    if(0 > mtd_open(&mdev, argv[1], O_RDWR)) {
        return -1;
    }

    struct crc_section_t *bootcrc, *fwcrc;
    char buffer[mdev.info.erasesize];

    if(0 > mtd_read_next_block(&mdev, buffer)) {
        mtd_close(&mdev);
        return -1;
    }

    bootcrc = (struct crc_section_t*)&buffer[BOOT_KFSINFO_OFFSET];
    fwcrc   = (struct crc_section_t*)&buffer[FW_KFSINFO_OFFSET];

    mtd_seek_block(&mdev, -1, SEEK_CUR);

    bootcrc->cksum[CFERAM_CRC_INDEX] = 0;
    memset(bootcrc, 0, sizeof(*bootcrc));
    fwcrc->cksum[FW_CRC_EN_INDEX] = 0;

    mtd_write_next_block(&mdev, buffer);

    mtd_close(&mdev);

    sync();

    return 0;
}
