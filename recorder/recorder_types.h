#ifndef RECTYPES_H
#define RECTYPES_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned short uint16_t;
typedef unsigned int UINT32;
typedef unsigned int UINT;
typedef unsigned long long UINT64;

typedef struct _field {
    unsigned char sz;
    unsigned char tp;
    char * nm;
} field;

#endif // RECTYPES_H
