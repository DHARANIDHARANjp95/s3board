

#ifndef __COMMON_H_
#define __COMMON_H_

#include "stdint.h"


#define SET_BIT(reg, n) (reg |= (1 << n))
#define CLR_BIT(reg, n) (reg &= ~(1 << n))
#define CHECK_BIT(reg, n) (reg & (1 << n))

#define LENGTH_ARRAY(x) (sizeof(x)/sizeof(x[0]))

uint32_t tickDiff(uint32_t startTime, uint32_t currentTime);
#endif
