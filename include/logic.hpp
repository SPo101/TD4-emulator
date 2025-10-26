#pragma once

#define tobool(num) \
  ((num) & 0x01 ? 1 : 0)

unsigned char not_(unsigned char num, int pos=0);

unsigned char neg(unsigned char num);
