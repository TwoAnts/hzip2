#ifndef __BIT_STREAM_H
#define __BIT_STREAM_H

#include <stdio.h>

//#define PRINT_BIT_REVERSE
#define PRINT_BIT_END ('\n')
void print_bit(char byte);

#define BS_MODE_READ  1
#define BS_MODE_WRITE 2

char mask_low(int len);
char merge_bit(char left, char right, int rightlen);

typedef struct bitstream {
    FILE *source;
    int mode;
    int len_bit;
    int offset_in_byte;
    char byte_buffer;
} *BitStream;

BitStream bs_create(FILE *source, int mode);
int bs_destroy(BitStream bs);

int bs_read(BitStream bs);
int bs_flush(BitStream bs);
int bs_write(BitStream bs, int bit);

#endif //__BIT_STREAM_H