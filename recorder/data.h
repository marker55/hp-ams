/////////////////////////////////////////////////////////////////////
//
//  data.h
//
//  iLO "flight data recorder"
//  Descriptor and data record definitions
//
//  Copyright 2011 by Hewlett-Packard Development Company, L.P.
//
/////////////////////////////////////////////////////////////////////

#ifndef INCL_RECORDER_DATA_H
#define INCL_RECORDER_DATA_H

#include <string.h>
#include "recorder_types.h"

#pragma pack(1)

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_API4_FLAGS
//
/////////////////////////////////////////////////////////////////////

union RECORDER_API4_FLAGS
{
    UINT8   AsBYTE;                     // Byte 0

    struct
    {
        UINT8   __Byte_00_Bit_0 : 1;    // Byte 0, Bit 0
        UINT8   __Byte_00_Bit_1 : 1;    // Byte 0, Bit 1
        UINT8   Scope : 2;              // Byte 0, Bit 2-3
        UINT8   __Byte_00_Bit_4 : 1;    // Byte 0, Bit 4
        UINT8   __Byte_00_Bit_5 : 1;    // Byte 0, Bit 5
        UINT8   __Byte_00_Bit_6 : 1;    // Byte 0, Bit 6
        UINT8   DescriptorRecord : 1;   // Byte 0, Bit 7
    };
};

typedef union RECORDER_API4_FLAGS RECORDER_API4_FLAGS;

// C_ASSERT(sizeof(RECORDER_API4_FLAGS) == 1);

// RECORDER_API4_FLAGS.Scope

enum REC_DESC_SCOPE
{
    REC_DESC_SCOPE_CODE              = 1,
    REC_DESC_SCOPE_CLASS             = 2,
    REC_DESC_SCOPE_FIELD             = 3
};

typedef enum  REC_DESC_SCOPE             REC_DESC_SCOPE;

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_API4
//
/////////////////////////////////////////////////////////////////////

#define BLACBOX_MAX_DESCRIPTION     88

struct RECORDER_API4
{
    RECORDER_API4_FLAGS Flags;    // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2
    UINT8       Field;                  // Byte 3

    UINT8       Severity : 4;           // Byte 4, Bits 0-3

    UINT8       DataType : 4;           // Byte 4, Bits 4-7

    UINT8       DataFormat;             // Byte 5

    UINT8       Visibility : 2;         // Byte 6, Bits 0-1
    UINT8       __Byte_06_Bit_2_7 : 6;  // Byte 6, Bit 4-6

    UINT8       Instance;              // Byte 7

    char        Description[BLACBOX_MAX_DESCRIPTION];   // Byte 8 - 95
};

typedef struct RECORDER_API4  RECORDER_API4;

// C_ASSERT(sizeof(RECORDER_API4) == 96);

// RECORDER_API4.Severity

enum REC_DESC_SEVERITY
{
    REC_DESC_SEVERITY_CRITICAL       = 1,
    REC_DESC_SEVERITY_NON_CRITICAL   = 2,
    REC_DESC_SEVERITY_INFORMATIONAL  = 3,
    REC_DESC_SEVERITY_PERIODIC       = 4,
    REC_DESC_SEVERITY_CONFIGURATION  = 5,
    REC_DESC_SEVERITY_ERROR          = 1,    // Same as REC_DESC_SEVERITY_CRITICAL
    REC_DESC_SEVERITY_WARNING        = 2,    // Same as REC_DESC_SEVERITY_NON_CRITICAL
    REC_DESC_SEVERITY_STATE_CHANGE   = 3     // Same as REC_DESC_SEVERITY_INFORMATIONAL
};

typedef enum   REC_DESC_SEVERITY     REC_DESC_SEVERITY;

// RECORDER_API4.DataType

enum REC_DESC_DATATYPE
{
    REC_DESC_DATATYPE_BINARY         = 0,
    REC_DESC_DATATYPE_TEXT           = 1,
    REC_DESC_DATATYPE_UNSIGNED_LSB   = 2,
    REC_DESC_DATATYPE_SIGNED_LSB     = 3,
    REC_DESC_DATATYPE_UNSIGNED_MSB   = 4,
    REC_DESC_DATATYPE_SIGNED_MSB     = 5,
    REC_DESC_DATATYPE_SIGNAL_BITS    = 6,
    REC_DESC_DATATYPE_NUMERIC        = 2     // Same as REC_DESC_DATATYPE_UNSIGNED_LSB
};

typedef enum   REC_DESC_DATATYPE     REC_DESC_DATATYPE;

// RECORDER_API4.DataFormat

enum REC_DESC_DATAFMT
{
    REC_DESC_DATAFMT_OTHER           = 0,
    REC_DESC_DATAFMT_BYTE            = 1,
    REC_DESC_DATAFMT_WORD            = 2,
    REC_DESC_DATAFMT_DWORD           = 4,
    REC_DESC_DATAFMT_QWORD           = 8
};

typedef enum   REC_DESC_DATAFMT      REC_DESC_DATAFMT;

// RECORDER_API4.Visibility

enum REC_DESC_VISIBILITY
{
    REC_DESC_VISIBILITY_RESERVED     = 0,
    REC_DESC_VISIBILITY_CUSTOMER     = 1,
    REC_DESC_VISIBILITY_SERVICE      = 2,
    REC_DESC_VISIBILITY_ENGENEERING  = 3
};

typedef enum   REC_DESC_VISIBILITY   REC_DESC_VISIBILITY;

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_BASE_API4
//  RECORDER_CODE_API4
//  RECORDER_FIELD_API4
//
//  C++ classes that make it easier to initialize descriptors
//
/////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

class RECORDER_BASE_API4 : public RECORDER_API4
{
    public:

        RECORDER_BASE_API4()
        {}

        RECORDER_BASE_API4(
            REC_DESC_SCOPE Scope,
            UINT Class,
            UINT Code,
            UINT Field,
            const char* Description,
            REC_DESC_SEVERITY Severity,
            REC_DESC_VISIBILITY Visibility,
            REC_DESC_DATATYPE DataType,
            REC_DESC_DATAFMT DataFormat
        )
        {
            Init(Scope,
                 Class,
                 Code,
                 Field,
                 Description,
                 Severity,
                 Visibility,
                 DataType,
                 DataFormat);
        }

        void
        Init(
            REC_DESC_SCOPE Scope,
            UINT Class,
            UINT Code,
            UINT Field,
            const char* Description,
            REC_DESC_SEVERITY Severity,
            REC_DESC_VISIBILITY Visibility,
            REC_DESC_DATATYPE DataType,
            REC_DESC_DATAFMT DataFormat
        )
        {
            *(UINT64*)this = 0;

            this->Flags.DescriptorRecord = 1;

            this->Flags.Scope   = (UINT8) Scope;

            this->Class         = (UINT8) Class;
            this->Code          = (UINT8) Code;
            this->Field         = (UINT8) Field;
            this->Severity      = (UINT8) Severity;
            this->DataType      = (UINT8) DataType;
            this->DataFormat    = (UINT8) DataFormat;
            this->Visibility    = (UINT8) Visibility;

            strncpy(this->Description, Description, BLACBOX_MAX_DESCRIPTION-1);
            this->Description[BLACBOX_MAX_DESCRIPTION-1] = 0;
        };
};

// C_ASSERT(sizeof(RECORDER_BASE_API4) == 96);

class RECORDER_CODE_API4 : public RECORDER_BASE_API4
{
    public:

        RECORDER_CODE_API4()
          : RECORDER_BASE_API4()
        {}

        RECORDER_CODE_API4(
            UINT Class,
            UINT Code,
            const char* Description,
            REC_DESC_SEVERITY Severity = REC_DESC_SEVERITY_NON_CRITICAL,
            REC_DESC_VISIBILITY Visibility = REC_DESC_VISIBILITY_CUSTOMER,
            REC_DESC_DATATYPE DataType = REC_DESC_DATATYPE_BINARY,
            REC_DESC_DATAFMT DataFormat = REC_DESC_DATAFMT_OTHER
        )
          : RECORDER_BASE_API4(
                REC_DESC_SCOPE_CODE,
                Class,
                Code,
                0,
                Description,
                Severity,
                Visibility,
                DataType,
                DataFormat
            )
        {}

        void
        Init(
            UINT Class,
            UINT Code,
            const char* Description,
            REC_DESC_SEVERITY Severity = REC_DESC_SEVERITY_NON_CRITICAL,
            REC_DESC_VISIBILITY Visibility = REC_DESC_VISIBILITY_CUSTOMER,
            REC_DESC_DATATYPE DataType = REC_DESC_DATATYPE_BINARY,
            REC_DESC_DATAFMT DataFormat = REC_DESC_DATAFMT_OTHER
        )
        {
            RECORDER_BASE_API4::Init(
                REC_DESC_SCOPE_CODE,
                Class,
                Code,
                0,
                Description,
                Severity,
                Visibility,
                DataType,
                DataFormat
            );
        }
};

// C_ASSERT(sizeof(RECORDER_CODE_API4) == 96);

class RECORDER_FIELD_API4 : public RECORDER_BASE_API4
{
    public:

        RECORDER_FIELD_API4()
          : RECORDER_BASE_API4()
        {}

        RECORDER_FIELD_API4(
            UINT Class,
            UINT Code,
            UINT Field,
            const char* Description,
            REC_DESC_SEVERITY Severity = REC_DESC_SEVERITY_NON_CRITICAL,
            REC_DESC_VISIBILITY Visibility = REC_DESC_VISIBILITY_CUSTOMER,
            REC_DESC_DATATYPE DataType = REC_DESC_DATATYPE_BINARY,
            REC_DESC_DATAFMT DataFormat = REC_DESC_DATAFMT_OTHER
        )
          : RECORDER_BASE_API4(
                REC_DESC_SCOPE_FIELD,
                Class,
                Code,
                Field,
                Description,
                Severity,
                Visibility,
                DataType,
                DataFormat
            )
        {}

        void
        Init(
            UINT Class,
            UINT Code,
            UINT Field,
            const char* Description,
            REC_DESC_SEVERITY Severity = REC_DESC_SEVERITY_NON_CRITICAL,
            REC_DESC_VISIBILITY Visibility = REC_DESC_VISIBILITY_CUSTOMER,
            REC_DESC_DATATYPE DataType = REC_DESC_DATATYPE_BINARY,
            REC_DESC_DATAFMT DataFormat = REC_DESC_DATAFMT_OTHER
        )
        {
            RECORDER_BASE_API4::Init(
                REC_DESC_SCOPE_FIELD,
                Class,
                Code,
                Field,
                Description,
                Severity,
                Visibility,
                DataType,
                DataFormat
            );
        }
};

// C_ASSERT(sizeof(RECORDER_FIELD_API4) == 96);

#endif

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_FLAGS
//
/////////////////////////////////////////////////////////////////////

union RECORDER_DATA_FLAGS
{
    UINT8   AsBYTE;                     // Byte 0

    struct
    {
        UINT8   __Byte_00_Bit_0 : 1;    // Byte 0, Bit 0
        UINT8   __Byte_00_Bit_1 : 1;    // Byte 0, Bit 1
        UINT8   IncludesField : 1;      // Byte 0, Bit 2
        UINT8   FullSnapshot : 1;       // Byte 0, Bit 3
        UINT8   Static : 1;             // Byte 0, Bit 4
        UINT8   IncludesTimetick : 1;   // Byte 0, Bit 5
        UINT8   IncludesInstance : 1;   // Byte 0, Bit 6
        UINT8   DescriptorRecord : 1;   // Byte 0, Bit 7
    };
};

typedef union RECORDER_DATA_FLAGS RECORDER_DATA_FLAGS;

// C_ASSERT(sizeof(RECORDER_DATA_FLAGS) == 1);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_SIMPLE
//
/////////////////////////////////////////////////////////////////////

struct RECORDER_DATA_SIMPLE
{
    RECORDER_DATA_FLAGS Flags;          // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2

    UINT16      Length;                 // Byte 3-4
    UINT8       Data[1];                // Byte 5+
};

typedef struct RECORDER_DATA_SIMPLE RECORDER_DATA_SIMPLE;

// C_ASSERT(sizeof(RECORDER_DATA_SIMPLE) == 6);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_WITH_FIELD_OR_INSTANCE
//
/////////////////////////////////////////////////////////////////////

struct RECORDER_DATA_WITH_FIELD_OR_INSTANCE
{
    RECORDER_DATA_FLAGS Flags;          // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2

    union
    {
        UINT8   Field;                  // Byte 3
        UINT8   Instance;               // Byte 3
    };

    UINT16      Length;                 // Byte 4-5
    UINT8       Data[1];                // Byte 6+
};

typedef struct RECORDER_DATA_WITH_FIELD_OR_INSTANCE RECORDER_DATA_WITH_FIELD_OR_INSTANCE;

// C_ASSERT(sizeof(RECORDER_DATA_WITH_FIELD_OR_INSTANCE) == 7);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_WITH_FIELD_AND_INSTANCE
//
/////////////////////////////////////////////////////////////////////

struct RECORDER_DATA_WITH_FIELD_AND_INSTANCE
{
    RECORDER_DATA_FLAGS Flags;          // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2

    UINT8       Field;                  // Byte 3
    UINT8       Instance;               // Byte 4

    UINT16      Length;                 // Byte 5-6
    UINT8       Data[1];                // Byte 7+
};

typedef struct RECORDER_DATA_WITH_FIELD_AND_INSTANCE RECORDER_DATA_WITH_FIELD_AND_INSTANCE;

// C_ASSERT(sizeof(RECORDER_DATA_WITH_FIELD_AND_INSTANCE) == 8);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_WITH_TIMETICK
//
/////////////////////////////////////////////////////////////////////

struct RECORDER_DATA_WITH_TIMETICK
{
    RECORDER_DATA_FLAGS Flags;          // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2

    UINT32      Timetick;               // Byte 3-6

    UINT16      Length;                 // Byte 7-8
    UINT8       Data[1];                // Byte 9+
};

typedef struct RECORDER_DATA_WITH_TIMETICK RECORDER_DATA_WITH_TIMETICK;

// C_ASSERT(sizeof(RECORDER_DATA_WITH_TIMETICK) == 10);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK
//
/////////////////////////////////////////////////////////////////////

struct RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK
{
    RECORDER_DATA_FLAGS Flags;          // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2

    union
    {
        UINT8   Field;                  // Byte 3
        UINT8   Instance;               // Byte 3
    };

    UINT32      Timetick;               // Byte 4-7

    UINT16      Length;                 // Byte 8-9
    UINT8       Data[1];                // Byte 10+
};

typedef struct RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK;

// C_ASSERT(sizeof(RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK) == 11);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_DATA_WITH_FIELD_AND_INSTANCE_AND_TIMETICK
//
/////////////////////////////////////////////////////////////////////

struct RECORDER_DATA_WITH_FIELD_AND_INSTANCE_AND_TIMETICK
{
    RECORDER_DATA_FLAGS Flags;          // Byte 0

    UINT8       Class;                  // Byte 1
    UINT8       Code;                   // Byte 2

    UINT8       Field;                  // Byte 3
    UINT8       Instance;               // Byte 4

    UINT32      Timetick;               // Byte 5-8

    UINT16      Length;                 // Byte 9-10
    UINT8       Data[1];                // Byte 11+
};

typedef struct RECORDER_DATA_WITH_FIELD_AND_INSTANCE_AND_TIMETICK RECORDER_DATA_WITH_FIELD_AND_INSTANCE_AND_TIMETICK;

// C_ASSERT(sizeof(RECORDER_DATA_WITH_FIELD_AND_INSTANCE_AND_TIMETICK) == 12);

/////////////////////////////////////////////////////////////////////
//
//  RECORDER_RECORD
//
/////////////////////////////////////////////////////////////////////

union RECORDER_RECORD
{
    RECORDER_DATA_FLAGS                                 Header;
    RECORDER_DATA_SIMPLE                                Simple;
    RECORDER_DATA_WITH_FIELD_OR_INSTANCE                WithField;
    RECORDER_DATA_WITH_FIELD_OR_INSTANCE                WithInstance;
    RECORDER_DATA_WITH_FIELD_AND_INSTANCE               WithFieldAndInstance;
    RECORDER_DATA_WITH_TIMETICK                         WithTimetick;
    RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK   WithFieldAndTimetick;
    RECORDER_DATA_WITH_FIELD_OR_INSTANCE_AND_TIMETICK   WithInstanceAndTimetick;
    RECORDER_DATA_WITH_FIELD_AND_INSTANCE_AND_TIMETICK  WithFieldAndInstanceAndTimetick;
    RECORDER_API4                                 Descriptor;
};

typedef union RECORDER_RECORD RECORDER_RECORD;

/////////////////////////////////////////////////////////////////////

#pragma pack()

#endif // INCL_RECORDER_DATA_H
