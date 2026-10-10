#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>

void Sha256(const void *data, size_t len, unsigned char out[32]);
void Sha256Hex(const void *data, size_t len, char out[65]);

#endif
