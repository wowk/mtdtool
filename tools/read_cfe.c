#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <mtdlib.h>


struct ubi_ec_hdr {
    uint32_t magic;
    uint8_t  version;
    uint8_t  padding1[3];
    uint64_t ec;
    uint32_t vid_hdr_offset;
    uint32_t data_offset;
    uint32_t image_seq;
    uint32_t padding[32];
    uint32_t hdr_crc;
} __attribute__((packed));

 struct ubi_vid_hdr {
        uint32_t  magic;
        uint8_t    version;
        uint8_t    vol_type;
        uint8_t    copy_flag;
        uint8_t    compat;
        uint32_t  vol_id;
        uint32_t  lnum;
        uint32_t  leb_ver;
        uint32_t  data_size;
        uint32_t  used_ebs;
        uint32_t  data_pad;
        uint32_t  data_crc;
        uint8_t    padding2[4];
        uint64_t  sqnum;
        uint8_t    padding3[12];
        uint32_t  hdr_crc;
 } __attribute__ ((packed));


int ubi_header(const char* file, struct ubi_ec_hdr* ec)
{
    int fd;
    int retval;

    fd = open(file, O_RDONLY);
    if ( fd < 0 ) {
        printf("failed to open image file <%s>: %m", file);
        return -errno;
    } else {
        struct stat st;
        lseek(fd, 0, SEEK_END);
        printf("%d\n", lseek(fd, 0, SEEK_CUR));
        close(fd);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    ubi_header(argv[1], NULL);

    return 0;
}
