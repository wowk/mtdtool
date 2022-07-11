#ifndef FLASHMAP_H_
#define FLASHMAP_H_

#include <stdlib.h>

struct flash_part_t {
    unsigned start;
    unsigned size;
};

struct flashmap_t {
    size_t part_num;
    struct flash_part_t part[0];
};

int flashmap_load(struct flashmap_t **flashmap);



#endif
