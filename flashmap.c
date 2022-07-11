#include "flashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int flashmap_load(struct flashmap_t **flashmap) {
    FILE *fp = fopen("/proc/mtd", "r");
    if(!fp) {
        printf("failed to open /proc/mtd : %m\n");
        return -errno;
    }
    
    size_t count = -1;
    char line[256] = "";
    while(fgets(line, sizeof(line), fp)) {
        count ++;
    }
    rewind(fp);
    if(count <= 0) {
        fclose(fp);
        return count;
    }   

    *flashmap = malloc(sizeof(struct flashmap_t)+sizeof(struct flash_part_t)*count);
    if(!(*flashmap)) {
        fclose(fp);
        return -ENOMEM;
    }

    int prev_start = 0;
    (*flashmap)->part_num = count;
    (*flashmap)->part[0].start = 0;
    (*flashmap)->part[0].size = 0;
    fgets(line, sizeof(line), fp);
    for(int i = 0 ; i < count ; i ++) {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*s%x%*s%*s", &(*flashmap)->part[i].size);
        (*flashmap)->part[i].start = prev_start;
        prev_start += (*flashmap)->part[i].size;
    } 

    fclose(fp);

    return 0;
}
