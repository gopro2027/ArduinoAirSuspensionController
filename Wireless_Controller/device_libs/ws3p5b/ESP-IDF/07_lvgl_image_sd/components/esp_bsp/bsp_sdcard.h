#pragma once

#include <stdio.h>

#include <sys/unistd.h>
#include <sys/stat.h>


#ifdef __cplusplus
extern "C"
{
#endif

void bsp_sdcard_init(void);
uint64_t bsp_sdcard_get_size(void);

#ifdef __cplusplus
}
#endif