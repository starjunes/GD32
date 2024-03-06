#ifndef TBOX_SEED2KEY_H
#define TBOX_SEED2KEY_H

#include <stdint.h>

/*******************************************************************************
| Function:     tbox_seed2key
| Description:  calculate the key based on the random seed value
| param:        u32Seed - seed value, large endian order
| return:       key value, large endian order
******************************************************************************/
uint32_t tbox_seed2key(uint32_t u32Seed);

#endif