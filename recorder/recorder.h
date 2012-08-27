#ifndef REC_H
#define REC_H

/*******************************************************************
 * Includes
 *******************************************************************/
#include "recorder_types.h"

/*******************************************************************
 * Defines
 *******************************************************************/
// Recorder Implementation / Specification Version
#define RECORDER_VERSION         (0x01)

// API return codes
#define RECORDER_OK              (0)
#define RECORDER_API_ERROR       (1)
#define RECORDER_GENERIC_ERROR  (-1)
// Detailed Error Codes
#define RECORDER_ERR_ARGS         (2)
#define RECORDER_ERR_FULL         (3)
#define RECORDER_ERR_MALLOC       (4)
#define RECORDER_ERR_HANDLE       (5)
#define RECORDER_ERR_BAD_SIZE     (6)
#define RECORDER_ERR_NO_FILTER    (7)
#define RECORDER_ERR_INVALID_PARM (8)

// Size #defines
#define RECORDER_MAX_FEEDERS      (128)
#define RECORDER_MAX_BLOB_SIZE   (962)
#define RECORDER_MAX_NAME          (32)
#define RECORDER_HASH_SIZE       (sizeof(unsigned int))

// Internal structures (used as input to _register routines)
// Status / Flags (_register)
#define REC_STATUS_IN_USE       (0x80000000)
#define REC_STATUS_T0_LOGGED    (0x40000000)
#define REC_STATUS_ARRAY        (0x08000000)
#define REC_STATUS_HASH         (0x04000000)
#define REC_STATUS_NO_COMPARE   (0x02000000)
#define REC_STATUS_OS_PIECE     (0x00800000)
#define REC_STATUS_STATIC       (0x00000010)   /* Same as REC_FLAGS_STAIC below! */
#define REC_STATUS_TIMESTAMP    (0x00000020)   /* Same as REC_FLAGS_STAIC below! */
// Lowest byte reserved - do not use (ORed with FLAGS on occasion)
// Internal ONLY


// Masks used internally for validating the handle/tag
#define RECORDER_TAG_MASK      (0xFFFF0000)
#define RECORDER_HANDLE_MASK   (0x0000FFFF)

// Filter (F_TYPE)
#define RECORDER_FILTER_NO_FILTER  (0)
#define RECORDER_FILTER_BYTE       (1)
#define RECORDER_FILTER_WORD       (2)
#define RECORDER_FILTER_DWORD      (4)

#define RECORDER_FILTER_SET      (0x01)

//
// FLAGS FIELD
// 
// External #defines
// FLAGS field
// Some of these apply to data and some to descriptors, and some to both!
#define REC_FLAGS_DESCRIPTOR             (0x80)    
// If DESCRIPTOR
#define REC_FLAGS_DESC_40                (0x40)    
#define REC_FLAGS_DESC_20                (0x20)
#define REC_FLAGS_DESC_10                (0x10)
// Enumeration
#define REC_FLAGS_DESC_CLASS             (0x08)
#define REC_FLAGS_DESC_CODE              (0x04)
#define REC_FLAGS_DESC_FIELD             (0x0C)
// reserved
#define REC_FLAGS_DESC_R_02              (0x02)        // avail
#define REC_FLAGS_DESC_R_01              (0x01)
//#define REC_FLAGS_RESERVED_BELOW             (0x01)
// If DATA
#define REC_FLAGS_DESCRIPTOR                  (0x80)    
#define REC_FLAGS_INSTANCE                    (0x40)    
#define REC_FLAGS_TICK_PRESENT                (0x20)
#define REC_FLAGS_STATIC                      (0x10)
#define REC_FLAGS_T0                          (0x08)
#define REC_FLAGS_OFFSET_PRESENT              (0x04)
#define REC_FLAGS_RESERVED                    (0x02)
#define REC_FLAGS_RESERVED_FUTURE_EXPANSION   (0x01)


// DESCRIPTOR: Severity + Type
// Type - bits 7-4
#define REC_TYP_BINARY           (0)
#define REC_TYP_TEXT             (1 << 4)    
#define REC_TYP_NUM_UNSIGNED_LSB (2 << 4)    
#define REC_TYP_NUM_SIGNED_LSB   (3 << 4)    
#define REC_TYP_NUM_UNSIGNED_MSB (4 << 4)    
#define REC_TYP_NUM_USIGNED_MSB  (5 << 4)    
#define REC_TYP_SIGNAL_BITS      (6 << 4)    
// Default
#define REC_TYP_NUMERIC          (REC_TYP_NUM_UNSIGNED_LSB)

// Severity - bits 3-0
#define REC_SEV_RSVD          (0)    
#define REC_SEV_CRIT          (1)    
#define REC_SEV_NONCRIT       (2)    
#define REC_SEV_INFORM        (3)    
#define REC_SEV_PERIOD        (4)    
#define REC_SEV_CONFIG        (5)    
#define REC_SEV_ERROR         (1)
#define REC_SEV_WARN          (2)
#define REC_SEV_CHANGE        (3)    
// todo?
//#define REC_TYP_INSTANCE      (1 << 7)    

// -------------------------
// Numeric Size 8-15 / Text/blob Length
// Enum in lower bits
#define REC_BINARY_DATA      (0)
#define REC_SIZE_BYTE        (1)    
#define REC_SIZE_WORD        (2)    
#define REC_SIZE_DWORD       (4)    
#define REC_SIZE_QWORD       (8)    
// High bits
// not sued//#define REC_SIZE_BITS        (0x80)    

// -------------------------
// Visibility 16-23
#define REC_VIS_RESERVED     (0x00)    
#define REC_VIS_CUSTOMER     (0x01)    
#define REC_VIS_SERVICE      (0x02)    
#define REC_VIS_ENGINEERING  (0x03)    

// -------------------------
// Reserved 34-31
// Reserved

// Code bits
//#define REC_CODE_DESCRIPTOR  (0x80000000)
//#define REC_CODE_T0          (0x40000000)


#define REC_DESCRIBE_CLASS    (1)
#define REC_DESCRIBE_CODE     (2)
#define REC_DESCRIBE_FIELD    (3)




//
/*******************************************************************
 * Defines
 *******************************************************************/
#define REC_NUMBER_DESC_STRINGS_TRACKED  (32)

/*******************************************************************
 * Structures (for internal management)
 *
 *******************************************************************/
#pragma pack(1)
// Filter - bits used to identify BYTE,WORD,DWORD that are checked/ignored.
typedef struct {
    UINT32 mask;
} RECORDER_FILTER;

#pragma pack(1)
/*******************************************************************
 * Data Structures (into Recorder storage area)
 *
 *******************************************************************/
// DESCRIPTOR
typedef struct {
    UINT8 flags;
    UINT8 classs;
    UINT8 code;
    UINT8 field;
    UINT8 severity_type;
    UINT8 format;
    UINT8 visibility;
    UINT8 rsvd;
    char  desc[88];
} descript;

// RECORDER
// - These enumerations are important as they come from the CHIF world too - don't change them!
typedef enum
{ 
    // recorder
    REC_API_1 = 1,
    REC_API_2,
    REC_API_3,
    REC_API_4,
    REC_API_5,
    REC_API_6,
    REC_API_7,
    REC_API_8,
    REC_API_9,
    REC_API_10,
    REC_API_11,
    REC_API_12,
    REC_API_13
} REC_RECORDER_TYPES;

/*******************************************************************
 * MESSAGE STRUCTURES
 *  
 * Request and Response structures passed across the VAS boundary
 * 
 * NOTE:
 * NOTE: Also passed via CHIF, so once decided, we can't change them!
 * NOTE:
 *  
 *******************************************************************/
#define REC_DATA_BUFFER_SIZE    (928)
//#define REC_DATA_BUFFER_SIZE    (432)

/*******************************************************************
 * Recorder Structures
 *******************************************************************/
#pragma pack(1)
//
// Recorder
//
typedef struct {
    char name[32];
    unsigned int  classs;
    unsigned int  flags;
    unsigned int  size;
    unsigned int  count;
} REC_1_MSG;

typedef struct {
    int  handle;
} REC_2_MSG;

typedef struct {
    int  handle;
    unsigned int modify_mask;
    unsigned int classs;
    unsigned int code;
    unsigned int flags;
} REC_3_MSG;

// REC_DETAIL_MSG.ModifyMask

enum REC_DETAIL_MODIFY_MASK
{
    REC_MODIFY_CLASS         = 0x01,
    REC_MODIFY_CODE          = 0x02,
    REC_MODIFY_FLAGS         = 0x04
};


typedef struct {
    int  handle;
    unsigned int  selection;
    unsigned int  size;
    descript toc;    // Interpreted as an array
} REC_4_MSG;

typedef struct {
    int    handle;
    int    type;
    unsigned int    bytes;
    UINT32 filter;
} REC_5_MSG;

typedef struct {
    int  handle;
    unsigned int size;
    unsigned int instance;
    unsigned int field;
    char data[RECORDER_MAX_BLOB_SIZE];
} REC_6_MSG;

// ALLOC
typedef struct {
    int  classs;
    char name[32];
} REC_13_MSG;

// ALL
typedef struct {
    int  subtype;
    union {
        REC_1_MSG    m1;
        REC_2_MSG    m2;
        REC_3_MSG    m3;
        REC_4_MSG    m4;
        REC_5_MSG    m5;
        REC_6_MSG    m6;
        REC_13_MSG   m13;
    } cmd;
} REC_MSG;

// RECORDER
typedef struct {
    int  handle;
} REC_RESPONSE_DATA;


/*******************************************************************
 * MESSAGE STRUCTURES
 *  
 * Request and Response structures passed across the VAS boundary
 *
 *******************************************************************/
typedef struct
{
    int type;
    union {
        REC_MSG     rec;
    } rec_msg;
} REC_MESSAGE;

//#pragma pack(4)

typedef struct
{
    int              rcode;
    union {
        REC_RESPONSE_DATA         returned;
    } rec_msg;
} REC_ANSWER;


/*******************************************************************
 * Package Header
 *  
 * This header is populated and pre-fixed to each file as it is
 * prepared for extraction as a package.
 * 
 * Refer to "Excavator Specification' 
 *  
 *******************************************************************/
#define REC_PACKAGE_MAGIC      ("PKG_")
#define REC_PACKAGE_VERSION    (0x0100)

#define REC_PKG_SOURCE_RSVD       (0x0000)
#define REC_PKG_SOURCE_EXCAVATOR  (0x0001)
#define REC_PKG_SOURCE_ILO_HTTP   (0x0002)
#define REC_PKG_SOURCE_ILO_RIBCL  (0x1003)

typedef struct {
    UINT32 signature;             // magic: PKG_
    UINT16 version;            // 0x0100
    UINT16 source;             // see above
    UINT32 filelength;         // length excluding header
    UINT32 reserved_field;
    UINT32 options;             //   
    char   name[64]; 
    UINT32 reserved[7];
    UINT32 checksum;            // adler-32 for validating header
} REC_PACKAGE_HEADER;

typedef struct _pkt_hdr {
    uint16_t pktsz;
    uint16_t seq;
    uint16_t cmd;
    uint16_t svc;
} pkt_hdr;

typedef struct _rec_req {
    pkt_hdr hdr;
    REC_MESSAGE msg;
} _req;

typedef struct _rec_resp {
    pkt_hdr hdr;
    REC_ANSWER msg;
} _resp;

#pragma pack()

/*******************************************************************
 * Externs
 *******************************************************************/

/*******************************************************************
 * Function Prototypes
 *******************************************************************/
//
#ifdef __cplusplus
extern "C" {
#endif

int rec_api13( const char* name );

int rec_api1( const char * name, 
                 unsigned int cls, 
                 unsigned int flags, 
                 unsigned int size, 
                 int * handle );

int rec_api1_array( const char * name, 
                 unsigned int cls, 
                 unsigned int flags, 
                 unsigned int size, 
                 unsigned int count, 
                 int * handle );

int rec_api2( int handle );

int rec_api4_class( int handle, 
                 unsigned int size, 
                 const descript * toc );

int rec_api4_code( int handle, 
                 unsigned int size,  
                 const descript * toc );

int rec_api4_field( int handle, 
                 unsigned int size, 
                 const descript * toc );

int rec_api3(  int handle, 
                 unsigned int code );

int rec_api5( int handle, 
                 int type, 
                 unsigned int bytes, 
                 const UINT32 * f );

int rec_api6( int handle, 
                 const char * data, 
                 unsigned int size );

int rec_api6_array( int handle, 
                 const char * data, 
                 unsigned int size );

int rec_api6_instance( int handle, 
                 unsigned int instance, 
                 const char * data, 
                 unsigned int size );

int rec_api6_static( int handle, 
                 const char * data, 
                 unsigned int size );

int rec_api6_field( int handle, 
                 unsigned int instance, 
                 unsigned int field, 
                 const char * data, 
                 unsigned int size );


#ifdef __cplusplus
   }
#endif

/*******************************************************************
 * End Of File
 *******************************************************************/
#endif

