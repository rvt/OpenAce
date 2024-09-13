#pragma once

#include <stdint.h>

/**
 * xxtea encryption
 * https://en.wikipedia.org/wiki/XXTEA
*/
void xxteaEncrypt(uint32_t data[], uint8_t words, const uint32_t key[4], uint8_t loops);
/**
 * xxtea decription
 * https://en.wikipedia.org/wiki/XXTEA
*/
void xxteaDecrypt(uint32_t data[], uint8_t words, const uint32_t key[4], uint8_t loops);

void TEA_Encrypt_Key0 (uint32_t* data, uint8_t loops);

void TEA_Decrypt_Key0 (uint32_t* data, uint8_t loops);


inline uint32_t XXTEA_MX_KEY0(uint32_t Y, uint32_t Z, uint32_t Sum)
{
    return ((((Z>>5) ^ (Y<<2)) + ((Y>>3) ^ (Z<<4))) ^ ((Sum^Y) + Z));
}

void XXTEA_Encrypt_Key0(uint32_t *Data, uint8_t Words, uint8_t Loops);

void XXTEA_Decrypt_Key0(uint32_t *Data, uint8_t Words, uint8_t Loops);