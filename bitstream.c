#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <string.h>
#else
#include <memory.h>
#endif

#define MASK_LOW(len) ((char)(~((0xff)>>(len)<<(len))))
#define MERGE_BIT(left, right, rightlen) ((char)(((~MASK_LOW(rightlen))&(left))|((MASK_LOW(rightlen))&(right))))

#define BS_BUFFER_SIZE 4096

typedef struct bitstream {
    FILE *source;
    int write;
    int len;
    int len_extra_bit;
    int offset;
    int offset_in_byte;
    char buffer[BS_BUFFER_SIZE];
} *BitStream;

int bs_create(BitStream *bs, FILE *source, int write)
{
    BitStream _bs;
    _bs = (BitStream)malloc(sizeof(struct bitstream));
    if(!_bs) return -1;
    _bs->source = source;
    _bs->write = write;
    _bs->len = 0;
    _bs->len_extra_bit = 0;
    _bs->offset = 0;
    _bs->offset_in_byte = 0;
    *bs = _bs;
    return 0;
}

int bs_destroy(BitStream bs)
{
    free(bs);
    return 0;
}

int bs_read(BitStream bs)
{
    if(bs->write) return -1;
    
    if(bs->offset_in_byte > 7)
    {
        bs->offset_in_byte = 0;
        bs->offset++;
    }
    
    if(bs->len <= 0 
        || bs->offset >= bs->len)
    {
        bs->len = fread(bs->buffer, sizeof(char), BS_BUFFER_SIZE,
                        bs->source);
        if(bs->len <= 0) return -1;
        bs->offset = 0;
        bs->offset_in_byte = 0;
    }
    
    return (bs->buffer[bs->offset]>>(bs->offset_in_byte++))&1;
}

int bs_flush(BitStream bs){
    if(!bs->write) return -1;
    
    char byte_source, byte;
    
    if(bs->offset_in_byte > 7)
    {
        bs->offset_in_byte = 0;
        bs->offset++;
    }
    
    if(bs->len_extra_bit > 0)
    {
        fseek(bs->source, -1, SEEK_CUR);
        byte_source = fgetc(bs->source);
        byte =  bs->buffer[bs->len];
        bs->buffer[bs->len] = MERGE_BIT(byte, byte_source, bs->len_extra_bit);
    }
    
    if(bs->offset_in_byte > 0)
    {
        assert(bs->offset < BS_BUFFER_SIZE);
        byte = bs->buffer[bs->offset];
        bs->buffer[bs->offset] = MERGE_BIT(0, byte, bs->offset_in_byte);
    }
    
    int len = bs->offset_in_byte?bs->offset+1:bs->offset;
    int wlen;
    wlen = fwrite(bs->buffer + bs->len, sizeof(char), len - bs->len, bs->source);
    
    if(wlen < len - bs->len)
    {
        bs->len_extra_bit = 0;
        bs->len += wlen;
    }else
    {
        bs->len_extra_bit = bs->offset_in_byte;
        bs->len += (bs->len_extra_bit?wlen-1:wlen);
        assert((bs->len_extra_bit)?(bs->len == (bs->offset)):(bs->len==bs->offset+1));
    }
    
    if(bs->len_extra_bit == 0 && bs->len >= BS_BUFFER_SIZE)
    {
        assert(bs->offset == BS_BUFFER_SIZE);
        bs->len = 0;
        bs->offset = 0;
    }
    
    return wlen;
}

int bs_write(BitStream bs, int bit){
    if(!bs->write) return -1;
    
    if(bs->offset_in_byte > 7)
    {
        bs->offset_in_byte = 0;
        bs->offset++;
    }
    
    if(bs->offset >= BS_BUFFER_SIZE){
        assert(bs->offset_in_byte == 0);
        bs_flush(bs);
        assert(bs->offset == 0);
    }
    
    bit = bit?0xff:0;
    bs->buffer[bs->offset] = MERGE_BIT(bit, bs->buffer[bs->offset], bs->offset_in_byte);
    return 1;
}

#define REVERSE
#define END ('\n')
void print_bit(char byte)
{
    char buffer[9];
    int i;
    buffer[8] = '\0';
    for(i = 7; i >= 0; i--)
    {
        #ifdef REVERSE
        buffer[i] = ((byte >> i)&1)?'1':'0';
        #else
        buffer[7-i] = ((byte >> i)&1)?'1':'0';
        #endif
    }
    printf("%s%c", buffer, END);
}

int main()
{
    //print_bit((char)0xff<<1);
    //print_bit(0xf0);
    //print_bit(MASK_LOW(3));
    //print_bit(MERGE_BIT(0b1001010, 0b01, 3));
    
    
    
    return 0;
}

