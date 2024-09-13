#include <stdint.h>
#include <string.h>

#include "ognconv.hpp"

// ==============================================================================================

uint16_t EncodeUR2V8(uint16_t Value)                                 // Encode unsigned 12bit (0..3832) as 10bit
{
    if(Value<0x100) { }
    else if(Value<0x300) Value = 0x100 | ((Value-0x100)>>1);
    else if(Value<0x700) Value = 0x200 | ((Value-0x300)>>2);
    else if(Value<0xF00) Value = 0x300 | ((Value-0x700)>>3);
    else                 Value = 0x3FF;
    return Value;
}

uint16_t DecodeUR2V8(uint16_t Value)                                 // Decode 10bit 0..0x3FF
{
    uint16_t  Range = Value>>8;
    Value &= 0x0FF;
    if(Range==0) return Value;              // 000..0FF
    if(Range==1) return 0x101+(Value<<1);   // 100..2FE
    if(Range==2) return 0x302+(Value<<2);   // 300..6FC
    return 0x704+(Value<<3);
} // 700..EF8                       // in 12bit (0..3832)


uint8_t EncodeUR2V5(uint16_t Value)                                  // Encode unsigned 9bit (0..472) as 7bit
{
    if(Value<0x020) { }
    else if(Value<0x060) Value = 0x020 | ((Value-0x020)>>1);
    else if(Value<0x0E0) Value = 0x040 | ((Value-0x060)>>2);
    else if(Value<0x1E0) Value = 0x060 | ((Value-0x0E0)>>3);
    else                 Value = 0x07F;
    return Value;
}

uint16_t DecodeUR2V5(uint16_t Value)                                 // Decode 7bit as unsigned 9bit (0..472)
{
    uint8_t Range = (Value>>5)&0x03;
    Value &= 0x1F;
    if(Range==0) { }                            // 000..01F
    else if(Range==1)
    {
        Value = 0x021+(Value<<1);    // 020..05E
    }
    else if(Range==2)
    {
        Value = 0x062+(Value<<2);    // 060..0DC
    }
    else
    {
        Value = 0x0E4+(Value<<3);    // 0E0..1D8 => max. Value = 472
    }
    return Value;
}

uint8_t EncodeSR2V5(int16_t Value)                                  // Encode signed 10bit (-472..+472) as 8bit
{
    uint8_t Sign=0;
    if(Value<0)
    {
        Value=(-Value);
        Sign=0x80;
    }
    Value = EncodeUR2V5(Value);
    return Value | Sign;
}

int16_t DecodeSR2V5( int16_t Value)                                // Decode
{
    int16_t Sign =  Value&0x80;
    Value = DecodeUR2V5(Value&0x7F);
    return Sign ? -Value: Value;
}

uint16_t EncodeUR2V6(uint16_t Value)                                // Encode unsigned 10bit (0..952) as 8 bit
{
    if(Value<0x040) { }
    else if(Value<0x0C0) Value = 0x040 | ((Value-0x040)>>1);
    else if(Value<0x1C0) Value = 0x080 | ((Value-0x0C0)>>2);
    else if(Value<0x3C0) Value = 0x0C0 | ((Value-0x1C0)>>3);
    else                 Value = 0x0FF;
    return Value;
}

uint16_t DecodeUR2V6(uint16_t Value)                                // Decode 8bit as unsigned 10bit (0..952)
{
    uint16_t Range  = (Value>>6)&0x03;
    Value &= 0x3F;
    if(Range==0) { }                            // 000..03F
    else if(Range==1)
    {
        Value = 0x041+(Value<<1);    // 040..0BE
    }
    else if(Range==2)
    {
        Value = 0x0C2+(Value<<2);    // 0C0..1BC
    }
    else
    {
        Value = 0x1C4+(Value<<3);    // 1C0..3B8 => max. Value = 952
    }
    return Value;
}

uint16_t EncodeSR2V6(int16_t Value)                                 // Encode signed 11bit (-952..+952) as 9bit
{
    uint16_t Sign=0;
    if(Value<0)
    {
        Value=(-Value);
        Sign=0x100;
    }
    Value = EncodeUR2V6(Value);
    return Value | Sign;
}

int16_t DecodeSR2V6( int16_t Value)                                // Decode 9bit as signed 11bit (-952..+952)
{
    int16_t Sign =  Value&0x100;
    Value = DecodeUR2V6(Value&0x00FF);
    return Sign ? -Value: Value;
}

uint8_t EncodeUR2V4(uint8_t DOP)
{
    if(DOP<0x10) { }
    else if(DOP<0x30) DOP = 0x10 | ((DOP-0x10)>>1);
    else if(DOP<0x70) DOP = 0x20 | ((DOP-0x30)>>2);
    else if(DOP<0xF0) DOP = 0x30 | ((DOP-0x70)>>3);
    else              DOP = 0x3F;
    return DOP;
}

uint8_t DecodeUR2V4(uint8_t DOP)
{
    uint8_t Range = DOP>>4;
    DOP &= 0x0F;
    if(Range==0) return       DOP;              // 00..0F
    if(Range==1) return 0x11+(DOP<<1);          // 10..2E
    if(Range==2) return 0x32+(DOP<<2);          // 30..6C
    return 0x74+(DOP<<3);
}        // 70..E8 => max. DOP = 232*0.1=23.2

uint16_t EncodeUR2V12(uint16_t Value)                        // encode unsigned 16-bit (0..61432) as 14-bit
{
    if(Value<0x1000) { }
    else if(Value<0x3000) Value = 0x1000 | ((Value-0x1000)>>1);
    else if(Value<0x7000) Value = 0x2000 | ((Value-0x3000)>>2);
    else if(Value<0xF000) Value = 0x3000 | ((Value-0x7000)>>3);
    else                  Value = 0x3FFF;
    return Value;
}

uint16_t DecodeUR2V12(uint16_t Value)
{
    uint16_t Range = Value>>12;
    Value &=0x0FFF;
    if(Range==0) return         Value;       // 0000..0FFF
    if(Range==1) return 0x1001+(Value<<1);   // 1000..2FFE
    if(Range==2) return 0x3002+(Value<<2);   // 3000..6FFC
    return 0x7004+(Value<<3);
} // 7000..EFF8 => max: 61432

// ==============================================================================================

uint8_t EncodeGray(uint8_t Binary)
{
    return Binary ^ (Binary>>1);
}

uint8_t DecodeGray(uint8_t Gray)
{
    Gray ^= (Gray >> 4);
    Gray ^= (Gray >> 2);
    Gray ^= (Gray >> 1);
    return Gray;
}

uint16_t EncodeGray(uint16_t Binary)
{
    return Binary ^ (Binary>>1);
}

uint16_t DecodeGray(uint16_t Gray)
{
    Gray ^= (Gray >> 8);
    Gray ^= (Gray >> 4);
    Gray ^= (Gray >> 2);
    Gray ^= (Gray >> 1);
    return Gray;
}

uint32_t EncodeGray(uint32_t Binary)
{
    return Binary ^ (Binary>>1);
}

uint32_t DecodeGray(uint32_t Gray)
{
    Gray ^= (Gray >>16);
    Gray ^= (Gray >> 8);
    Gray ^= (Gray >> 4);
    Gray ^= (Gray >> 2);
    Gray ^= (Gray >> 1);
    return Gray;
}

// ==============================================================================================

void XorShift32(uint32_t &Seed)      // simple random number generator
{
    Seed ^= Seed << 13;
    Seed ^= Seed >> 17;
    Seed ^= Seed << 5;
}

void XorShift64(uint64_t &Seed)
{
    Seed ^= Seed >> 12;
    Seed ^= Seed << 25;
    Seed ^= Seed >> 27;
}

// ==============================================================================================

static const unsigned char MapAscii85[86] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";

static const uint8_t UnmapAscii85[128] =
{
    85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
    85, 62, 85, 63, 64, 65, 66, 85, 67, 68, 69, 70, 85, 71, 85, 85,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 85, 72, 73, 74, 75, 76,
    77, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 85, 85, 85, 78, 79,
    80, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 81, 82, 83, 84, 85
};

uint8_t EncodeAscii85(char *Ascii, uint32_t Word)
{
    for( uint8_t Idx=5; Idx; )
    {
        uint32_t Div = Word/85;
        Idx--;
        Ascii[Idx]=MapAscii85[Word-Div*85];
        Word=Div;
    }
    Ascii[5]=0;
    return 5;
}

uint8_t DecodeAscii85(uint32_t &Word, const char *Ascii)
{
    Word=0;
    for( uint8_t Idx=0; Idx<5; Idx++)
    {
        char Char = Ascii[Idx];
        if(Char<=0) return 0;
        uint8_t Dig = UnmapAscii85[(uint8_t)Char];
        if(Dig>=85) return 0;
        Word = Word*85+Dig;
    }
    return 5;
}
