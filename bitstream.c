#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <string.h>
#else
#include <memory.h>
#endif

//#define REVERSE
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


#define BS_MODE_WRITE 2
#define BS_MODE_READ  1

char mask_low(int len)
{
    return ((char)(~((0xff)>>(len)<<(len))));
}

char merge_bit(char left, char right, int rightlen)
{
    return ((char)(((~mask_low(rightlen))&(left))|((mask_low(rightlen))&(right))));
}

typedef struct bitstream {
    FILE *source;
    int mode;
    int len_bit;
    int offset_in_byte;
    char byte_buffer;
} *BitStream;

BitStream bs_create(FILE *source, int mode)
{
    BitStream _bs;
    _bs = (BitStream)malloc(sizeof(struct bitstream));
    if(!_bs) return NULL;
    _bs->source = source;
    _bs->mode = mode;
    _bs->len_bit = 0;
    _bs->offset_in_byte = 0;
    return _bs;
}

int bs_destroy(BitStream bs)
{
    free(bs);
    return 0;
}

int bs_read(BitStream bs)
{
    if(!(bs->mode & BS_MODE_READ)) return -1;
    
    
    if(bs->offset_in_byte > 7)
    {
        bs->offset_in_byte = 0;
        bs->len_bit = 0;
    }
    
    if(bs->len_bit <= 0)
    {
        bs->byte_buffer = fgetc(bs->source);
        if(bs->byte_buffer == EOF)
            return -1;
        bs->len_bit = 8;
    }
    
    return (bs->byte_buffer>>(bs->offset_in_byte++))&1;
}

int bs_flush(BitStream bs)
{
    if(!(bs->mode & BS_MODE_WRITE)) return -1;
    
    char byte_source, byte;
    int bit_written;
    printf("len_bit_%d \n", bs->len_bit);
    printf("offset_%d\n", bs->offset_in_byte);
    if(bs->len_bit == bs->offset_in_byte) return 0;
    
    if(bs->len_bit > 0)
    {
        if(fseek(bs->source, -1, SEEK_CUR))
            return -1;
        byte_source = fgetc(bs->source);
        byte = bs->byte_buffer;
        bs->byte_buffer = merge_bit(byte, byte_source, bs->len_bit);
        if(fseek(bs->source, -1, SEEK_CUR))
            return -1;
    }
    
    if(bs->offset_in_byte > 7)
    {
        bs->offset_in_byte = 0;
        bit_written = 8 - bs->len_bit;
    }
    else if(bs->offset_in_byte > 0)
    {
        bs->byte_buffer = merge_bit(0, bs->byte_buffer, bs->offset_in_byte);
        bit_written = bs->offset_in_byte - bs->len_bit;
    }
    
    print_bit(bs->byte_buffer);
    char r;
    r = (char)fputc(bs->byte_buffer, bs->source);  //It will return the char written.
    if(r != bs->byte_buffer)
    {
        perror("fputc");
        return -1;
    }
    
    bs->len_bit = bs->offset_in_byte;
    
    return bit_written;
}

int bs_write(BitStream bs, int bit){
    if(!(bs->mode & BS_MODE_WRITE)) return -1;
    int bit_flushed;
    if(bs->offset_in_byte > 7)
    {
        if((bit_flushed = bs_flush(bs)) < 0)
            return -1;
        assert(bit_flushed == (8 - bs->len_bit));
    }
    
    bit = (char)((!!bit) << bs->offset_in_byte);
    bs->byte_buffer = merge_bit(bit, bs->byte_buffer, bs->offset_in_byte++);
    return 1;
}

void write_byte(char byte, BitStream bs)
{
    int i;
    for(i = 0;i < 8;i++)
    {
        bs_write(bs, (byte>>i)&1);
        bs_flush(bs);
    }
    bs_flush(bs);
}

int main()
{
    //print_bit((char)0xff<<1);
    //print_bit(0xf0);
    //print_bit(MASK_LOW(3));
    //print_bit(merge_bit(0b1001010, 0b01, 3));
    FILE *f = fopen("test.dat", "wb+");
    if(!f) return -1;
    
    BitStream bs = bs_create(f, BS_MODE_WRITE);
    
    /*fputc('a', f);
    fseek(f, -1, SEEK_CUR);
    printf("%c\n", fgetc(f));
    fseek(f, -1, SEEK_CUR);
    printf("%c\n", fputc('b', f));
    fseek(f, -1, SEEK_CUR);
    printf("%c\n", fgetc(f));*/
    write_byte(0xff, bs);
    //write_byte(0x0f, bs);
    //write_byte(0x81, bs);
    
    bs_destroy(bs);
    
    fclose(f);
    
    f = fopen("test.dat", "rb");
    if(!f) return -1;
    
    bs = bs_create(f, BS_MODE_READ);
    
    int bit;
    for(;;)
    {
        bit = bs_read(bs);
        if(bit == -1) break;
        printf("%c", bit?'1':'0');
    }
    
    bs_destroy(bs);
    
    fclose(f);
    
    return 0;
}

