#ifndef _SMBIOS_H
#define _SMBIOS_H

#include <sys/types.h>
#pragma pack(1)

/*
 * This include file conforms to version 2.7 prelim of the SM BIOS Spec.
 */

#define SMBIOS_ANCHOR_STRING                    "_SM_"
#define SMBIOS_EPS_ADDR_START                   0x0F0000
#define SMBIOS_EPS_ADDR_END                     0x0FFFFF
#define SMBIOS_EPS_BOUNDRY                      16
#define SMBIOS_EPS_SCAN_SIZE                    (SMBIOS_EPS_ADDR_END - SMBIOS_EPS_ADDR_START + 1)

#define SMBIOS_BIOS_INFO                        0
#define SMBIOS_SYSTEM_INFO                      1
#define SMBIOS_BASE_BOARD_INFO                  2
#define SMBIOS_SYSTEM_ENCLOSURE                 3
#define SMBIOS_PROCESSOR_INFO                   4
#define SMBIOS_MEMORY_CTLR_INFO                 5
#define SMBIOS_MEMORY_MODULE_INFO               6
#define SMBIOS_CACHE_INFO                       7
#define SMBIOS_PORT_CONNECTOR_INFO              8
#define SMBIOS_SYS_SLOTS                        9
#define SMBIOS_ON_BOARD_DEVS_INFO               10
#define SMBIOS_OEM_STRINGS                      11
#define SMBIOS_SYS_CFG_OPTIONS                  12
#define SMBIOS_BIOS_LANGUAGE_INFO               13
#define SMBIOS_GROUP_ASSOCIATIONS               14
#define SMBIOS_SYSTEM_EVENT_LOG                 15
#define SMBIOS_PHYS_MEM_ARRAY                   16
#define SMBIOS_MEM_DEVICE                       17
#define SMBIOS_MEMORY_ERROR_INFO                18
#define SMBIOS_MEM_ARR_MAPPED_ADDR              19
#define SMBIOS_MEM_DEV_MAPPED_ADDR              20
#define SMBIOS_BUILTIN_PT_DEVICE                21
#define SMBIOS_PORTABLE_BATTERY                 22
#define SMBIOS_SYS_RESET                        23
#define SMBIOS_HARDWARE_SECURITY                24
#define SMBIOS_SYSTEM_POWER_CONTROLS            25
#define SMBIOS_VOLTAGE_PROBE                    26
#define SMBIOS_COOLING_DEVICE                   27
#define SMBIOS_TEMERATURE_PROBE                 28
#define SMBIOS_ELECTRIC_CURRENT_PROBE           29
#define SMBIOS_OUT_OF_BAND_RAS                  30
#define SMBIOS_BIS_ENTRYPOINT                   31
#define SMBIOS_SYSTEM_BOOT_INFO                 32
#define SMBIOS_64BIT_MEM_ERROR_INFO             33
#define SMBIOS_MANAGEMENT_DEVICE                34
#define SMBIOS_MANAGEMENT_DEVICE_COMPONENT      35
#define SMBIOS_MANAGEMENT_DEVICE_THRESHOLD_DATA 36
#define SMBIOS_MEMORY_CHANNEL_DATA              37
#define SMBIOS_IPMI_DEVICE_INFO_DATA            38
#define SMBIOS_SYSTEM_POWER_SUPPLY_DATA         39
#define SMBIOS_SMBIOS_ADDITIONAL_INFO           40
#define SMBIOS_SMBIOS_ONBOARD_DEVICES_EX        41
#define SMBIOS_INACTIVE                         126
#define SMBIOS_END_OF_TABLE                     127
#define SMBIOS_HPOEM_SYSID                      195


#define SmbOffsetOfMember(s,m)   ((size_t)(&(((s *)0)->m)))
#define SmbSizeOfMember(s,m)     (sizeof((((s *)0)->m)))
#define SmbFieldValid(r,s,m)     ((r)->Header.byLength >= SmbOffsetOfMember(s,m) + SmbSizeOfMember(s,m))
#undef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

typedef struct _SMBIOS_HEADER
{
   u_char    byType;
   u_char    byLength;
   u_short    wHandle;
} SMBIOS_HEADER, *PSMBIOS_HEADER;

/* type 0 */
typedef struct _SMBIOS_BIOS_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byVendor;                       /* string # */
   u_char    byBIOSVersion;                  /* string # */
   u_short    wStartSegment;                  /* segment location of BIOS starting address */
   u_char    byReleaseDate;                  /* string # */
   u_char    byROMSize;                      /* 64K * (n + 1) bytes */
   /*u_int   dwCharacteristics1; */            /* first u_int of characteristics */
   /*u_int   dwCharacteristics2; */            /* second u_int of characteristics */
   u_int   bfReserved1           : 1;      /* Bit  0 */
   u_int   bfReserved2           : 1;      /* Bit  1 */
   u_int   bfUnknown             : 1;      /* Bit  2 */
   u_int   bfBIOSCharUnknown     : 1;      /* Bit  3 */
   u_int   bfISASupported        : 1;      /* Bit  4 */
   u_int   bfMCASupported        : 1;      /* Bit  5 */
   u_int   bfEISASupported       : 1;      /* Bit  6 */
   u_int   bfPCISupported        : 1;      /* Bit  7 */
   u_int   bfPCCardSupported     : 1;      /* Bit  8 */
   u_int   bfPnPSupported        : 1;      /* Bit  9 */
   u_int   bfAPMSupported        : 1;      /* Bit 10 */
   u_int   bfBIOSFlash           : 1;      /* Bit 11 */
   u_int   bfBIOSShadow          : 1;      /* Bit 12 */
   u_int   bfVLVesaSupported     : 1;      /* Bit 13 */
   u_int   bfESCDSupported       : 1;      /* Bit 14 */
   u_int   bfCDBootSupported     : 1;      /* Bit 15 */
   u_int   bfSelectableBoot      : 1;      /* Bit 16 */
   u_int   bfBIOSRomSocketed     : 1;      /* Bit 17 */
   u_int   bfPCMCIABoot          : 1;      /* Bit 18 */
   u_int   bfEDDSupported        : 1;      /* Bit 19 */
   u_int   bfInt13NEC            : 1;      /* Bit 20 */
   u_int   bfInt13Toshiba        : 1;      /* Bit 21 */
   u_int   bfInt13Floppy360      : 1;      /* Bit 22 */
   u_int   bfInt13Floppy12       : 1;      /* Bit 23 */
   u_int   bfInt13Floppy720      : 1;      /* Bit 24 */
   u_int   bfInt13Floppy288      : 1;      /* Bit 25 */
   u_int   bfInt5PrintScreen     : 1;      /* Bit 26 */
   u_int   bfInt9Keyboard        : 1;      /* Bit 27 */
   u_int   bfInt14Serial         : 1;      /* Bit 28 */
   u_int   bfInt17Printer        : 1;      /* Bit 29 */
   u_int   bfInt10CGAMono        : 1;      /* Bit 30 */
   u_int   bfNECPC98             : 1;      /* Bit 31 */
   u_int   bfReservedBIOSVendor  :16;      /* Bits 32:47 */
   u_int   bfReservedSystemVendor:16;      /* Bits 48:63 */
   /*u_char    byExtension; */                   /* extension byte */
   u_char    bfACPISupported       : 1;     /* Bit  0 */
   u_char    bfUSBSupported        : 1;     /* Bit  1 */
   u_char    bfAGPSupported        : 1;     /* Bit  2 */
   u_char    bfI2OSupported        : 1;     /* Bit  3 */
   u_char    bfLS120Supported      : 1;     /* Bit  4 */
   u_char    bfZIPSupported        : 1;     /* Bit  5 */
   u_char    bf1394Supported       : 1;     /* Bit  6 */
   u_char    bfSmartBatSupported   : 1;     /* Bit  7 */
   /*u_char    byExtension2; */                   /* extension byte 2 */
   u_char    bfBiosBootSpec        : 1;     /* Bit  0 */
   u_char    bfFKeyNetSrvcBoot     : 1;     /* Bit  1 */
   u_char    bfTargetedContent     : 1;     /* Bit  2 */
   u_char    bfReservedExtension2  : 5;     /* Bits 3:7 */
   u_char    bySystemBiosMajor;             /* System BIOS major release */
   u_char    bySystemBiosMinor;             /* System BIOS minor release */
   u_char    byEmbeddedCtrlrMajor;          /* Embedded controller major release */
   u_char    byEmbeddedCtrlrMinor;          /* Embedded controller minor release */
} SMBIOS_BIOS_INFORMATION, *PSMBIOS_BIOS_INFORMATION;

/* type 1 */
enum Wakeuptype_t
{
   wtReserved             = 0x00,
   wtOther                = 0x01,
   wtUnknown              = 0x02,
   wtAPMTimer             = 0x03,
   wtModemRing            = 0x04,
   wtLANRemote            = 0x05,
   wtPowerSwitch          = 0x06,
   wtPCIPME               = 0x07,
   wtACPowerRestored      = 0x08,
};
typedef struct _SMBIOS_SYSTEM_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byManufacturer;                 /* string # */
   u_char    byProductName;                  /* string # */
   u_char    byVersion;                      /* string # */
   u_char    bySerialNumber;                 /* string # */
   u_char    sUUID[16];                      /* UUID */
   u_char    byWakeupType;                   /* enumeration */
   u_char    bySKUNumber;                    /* string # */
   u_char    byFamily;                       /* string # */
} SMBIOS_SYSTEM_INFORMATION, *PSMBIOS_SYSTEM_INFORMATION;

/* type 2 */
enum Boardtype_t
{
   btUnknown              = 0x01,
   btOther                = 0x02,
   btServerBlade          = 0x03,
   btConnectivitySwitch   = 0x04,
   btSysMgmtModule        = 0x05,
   btProcModule           = 0x06,
   btIOModule             = 0x07,
   btMemoryModule         = 0x08,
   btDaughterBoard        = 0x09,
   btMotherboard          = 0x0A,
   btProcMemModule        = 0x0B,
   btProcIOModule         = 0x0C,
   btInterconnectBoard    = 0x0D,
};
typedef struct _SMBIOS_BASE_BOARD_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byManufacturer;                 /* string */
   u_char    byProduct;                      /* string */
   u_char    byVersion;                      /* string */
   u_char    bySerialNumber;                 /* string */
   u_char    byAssetTag;                     /* string */
   /*u_char    byFeatureFlags; */            /* feature flags */
   u_char    bfHostingBoard         : 1;     /* Bit  0 */
   u_char    bfReqDaughterBrd       : 1;     /* Bit  1 */
   u_char    bfRemoveable           : 1;     /* Bit  2 */
   u_char    bfReplaceable          : 1;     /* Bit  3 */
   u_char    bfHotSwappable         : 1;     /* Bit  4 */
   u_char    bfReservedExtension1   : 3;     /* Bits 5:7 */
   u_char    byLocationInChassis;            /* string */
   u_char    wType3ChassisHandle;            /* Handle to type 3 chassis record */
   u_char    byBoardType;                    /* enumeration */
   u_char    byNumContainedObjHandles;       /* u_short */
   u_short    wContainedObjHandles[1];        /* Open ended array of contained object handles */
} SMBIOS_BASE_BOARD_INFORMATION, *PSMBIOS_BASE_BOARD_INFORMATION;

/* type 3 */
enum ChassisType_t
{
   ctOther                 = 0x01,
   ctUnknown               = 0x02,
   ctDesktop               = 0x03,
   ctLowProfileDesktop     = 0x04,
   ctPizzaBox              = 0x05,
   ctMiniTower             = 0x06,
   ctTower                 = 0x07,
   ctPortable              = 0x08,
   ctLapTop                = 0x09,
   ctNotebook              = 0x0A,
   ctHandHeld              = 0x0B,
   ctDockingStation        = 0x0C,
   ctAllInOne              = 0x0D,
   ctSubNotebook           = 0x0E,
   ctSpaceSaving           = 0x0F,
   ctLunchBox              = 0x10,
   ctMainServerChassis     = 0x11,
   ctExpansionChassis      = 0x12,
   ctSubChassis            = 0x13,
   ctBusExpansionChassis   = 0x14,
   ctPeripheralChassis     = 0x15,
   ctRAIDChassis           = 0x16,
   ctRackMountChassis      = 0x17,
   ctSealedCasePC          = 0x18,
   ctMultiSystemChassis    = 0x19,
   ctCompactPCI            = 0x1A,
   ctAdvancedTCA           = 0x1B,
   ctBlade                 = 0x1C,
   ctBladeEnclosure        = 0x1D
};
enum ChassisState_t
{
   chsOther                = 0x01,
   chsUnknown              = 0x02,
   chsSafe                 = 0x03,
   chsWarning              = 0x04,
   chsCritical             = 0x05,
   chsNonRecoverable       = 0x06
};
enum ChassisSecurityStatus_t
{
   cssOther                         = 0x01,
   cssUnknown                       = 0x02,
   cssNone                          = 0x03,
   cssExternalInterfaceLockedOut    = 0x04,
   cssExternalInterfaceEnabled      = 0x05
};
typedef struct _SMBIOS_SYSTEM_ENCLOSURE_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byManufacturer;                 /* string */
   /*u_char    byType; */                        /* see definitions for bit 7 and bits 6:0 */
   u_char    bfType             : 7;         /* Chassis type enumeration */
   u_char    bfLock             : 1;         /* Chassis lock present if 1 */
   u_char    byVersion;                      /* string */
   u_char    bySerialNumber;                 /* string */
   u_char    byAssetTag;                     /* string */
   u_char    byBootupState;                  /* enumeration */
   u_char    byPowerSupplyState;             /* enumeration */
   u_char    byThermalState;                 /* enumeration */
   u_char    bySecurityStatus;               /* enumeration */
   u_int   dwOEMDefined;                   /* OEM defined field */
   u_char    byHeight;                       /* Height in U's */
   u_char    byNumPwrCords;                  /* Number of power cords */
   u_char    byContainedElementCount;        /* Number of contained element records */
   struct
   {
      u_char    bfContainedElementType        : 7;      /* Contained element type (board type or structure type) */
      u_char    bfContainedElementTypeSelect  : 1;      /* Type select (1==structure type; 0==base board type) */
      u_char    byContainedElementMin;       /* Contained element minimum */
      u_char    byContainedElementMax;       /* Contained element maximum */
   } ele[1];
} SMBIOS_SYSTEM_ENCLOSURE_INFORMATION, *PSMBIOS_SYSTEM_ENCLOSURE_INFORMATION;

/* type 4 */
enum ProcessorType_t
{
   ptOther         =  1,
   ptUnknown       =  2,
   ptCentral       =  3,
   ptMath          =  4,
   ptDSP           =  5,
   ptVideo         =  6
};
enum Family_t
{
   pfOther                 = 0x01,
   pfUnknown               = 0x02,
   pf8086                  = 0x03,
   pf80286                 = 0x04,
   pf80386                 = 0x05,
   pf80486                 = 0x06,
   pf8087                  = 0x07,
   pf80287                 = 0x08,
   pf80387                 = 0x09,
   pf80487                 = 0x0a,
   pfPentium               = 0x0b,
   pfPentiumPro            = 0x0c,
   pfPentiumII             = 0x0d,
   pfPentiumMMX            = 0x0e,
   pfCeleron               = 0x0f,
   pfPentiumIIXeon         = 0x10,
   pfPentiumIII            = 0x11,
   pfM1                    = 0x12,
   pfM2                    = 0x13,
   pfCeleronM              = 0x14,
   pfPentium4HT            = 0x15,
   pfAMDDuron              = 0x18,
   pfK5                    = 0x19,
   pfK6                    = 0x1A,
   pfK6_2                  = 0x1B,
   pfK6_3                  = 0x1C,
   pfAMDAthlon             = 0x1D,
   pfAMD2900               = 0x1E,
   pfK6_2Plus              = 0x1F,
   pfPowerPC               = 0x20,
   pfPowerPC601            = 0x21,
   pfPowerPC603            = 0x22,
   pfPowerPC603Plus        = 0x23,
   pfPowerPC604            = 0x24,
   pfPowerPC620            = 0x25,
   pfPowerPCx704           = 0x26,
   pfPowerPC750            = 0x27,
   pfCoreDuo               = 0x28,
   pfCoreDuoM              = 0x29,
   pfCoreSoloM             = 0x2a,
   pfAtom                  = 0x2b,
   pfAlpha                 = 0x30,
   pfAlpha21064            = 0x31,
   pfAlpha21066            = 0x32,
   pfAlpha21164            = 0x33,
   pfAlpha21164PC          = 0x34,
   pfAlpha21164a           = 0x35,
   pfAlpha21264            = 0x36,
   pfAlpha21364            = 0x37,
   pfAmdUMobileM           = 0x38,
   pfAmdMobileM            = 0x39,
   pfAmdAthlDCM            = 0x3a,
   pfAmdOpt6100            = 0x3b,
   pfAmdOpt4100            = 0x3c,
   pfAmdOpt6200            = 0x3d,
   pfAmdOpt4200            = 0x3e,
   pfMIPS                  = 0x40,
   pfMIPSR4000             = 0x41,
   pfMIPSR4200             = 0x42,
   pfMIPSR4400             = 0x43,
   pfMIPSR4600             = 0x44,
   pfMIPSR10000            = 0x45,
   pfAmdCseries            = 0x46,
   pfAmdEseries            = 0x47,
   pfAmdSseries            = 0x48,
   pfAmdGseries            = 0x49,
   pfSPARC                 = 0x50,
   pfSuperSPARC            = 0x51,
   pfMicroSPARCII          = 0x52,
   pfMicroSPARCIIep        = 0x53,
   pfUltraSPARC            = 0x54,
   pfUltraSPARCII          = 0x55,
   pfUltraSPARCIIi         = 0x56,
   pfUltraSPARCIII         = 0x57,
   pfUltraSPARCIIII        = 0x58,
   pf68040                 = 0x60,
   pf68xxx                 = 0x61,
   pf68000                 = 0x62,
   pf68010                 = 0x63,
   pf68020                 = 0x64,
   pf68030                 = 0x65,
   pfHobbit                = 0x70,
   pfCrusoeTM5000          = 0x78,
   pfCrusoeTM3000          = 0x79,
   pfEfficeonTM8000        = 0x7a,
   pfWeitek                = 0x80,
   pfItanium               = 0x82,
   pfAthlon64              = 0x83,
   pfOpteron               = 0x84,
   pfSempron               = 0x85,
   pfTurion64              = 0x86,
   pfDCOpteron             = 0x87,
   pfAthlon64X2DC          = 0x88,
   pfTurion64X2Mobile      = 0x89,
   pfQCOpteron             = 0x8a,
   pf3GOpteron             = 0x8b,
   pfPhenomFXQC            = 0x8c,
   pfPhenomX4QC            = 0x8d,
   pfPhenomX2DC            = 0x8e,
   pfAthlonX2DC            = 0x8f,
   pfPA_RISC               = 0x90,
   pfPA_RISC_8500          = 0x91,
   pfPA_RISC_8000          = 0x92,
   pfPA_RISC_7300LC        = 0x93,
   pfPA_RISC_7200          = 0x94,
   pfPA_RISC_7100LC        = 0x95,
   pfPA_RISC_7100          = 0x96,
   pfV30                   = 0xa0,
   pfQCXeon3200            = 0xa1,
   pfDCXeon3000            = 0xa2,
   pfQCXeon5300            = 0xa3,
   pfDCXeon5100            = 0xa4,
   pfDCXeon5000            = 0xa5,
   pfDCXeonLV              = 0xa6,
   pfDCXeonULV             = 0xa7,
   pfDCXeon7100            = 0xa8,
   pfQCXeon5400            = 0xa9,
   pfQCXeon                = 0xaa,
   pfDCXeon5200            = 0xab,
   pfDCXeon7200            = 0xac,
   pfQCXeon7300            = 0xad,
   pfQCXeon7400            = 0xae,
   pfMCXeon7400            = 0xaf,
   pfPentiumIIIXeon        = 0xb0,
   pfPentiumIIISpeedStep   = 0xb1,
   pfPentium4              = 0xb2,
   pfXeon                  = 0xb3,
   pfAS400                 = 0xb4,
   pfXeonMP                = 0xb5,
   pfAthlonXP              = 0xb6,
   pfAthlonMP              = 0xb7,
   pfItanium2              = 0xb8,
   pfPentiumM              = 0xb9,
   pfCeleronD              = 0xba,
   pfPentiumD              = 0xbb,
   pfPentiumExtremeEdition = 0xbc,
   pfCoreSolo              = 0xbd,
   pfCore2Duo              = 0xbf,
   pfCore2Solo             = 0xc0,
   pfCore2Extreme          = 0xc1,
   pfCore2QC               = 0xc2,
   pfCore2ExtremeMobile    = 0xc3,
   pfCore2DuoMobile        = 0xc4,
   pfCore2SoloMobile       = 0xc5,
   pfCore2i7               = 0xc6,
   pfDCCeleron             = 0xc7,
   pfIBM390                = 0xc8,
   pfG4                    = 0xc9,
   pfG5                    = 0xca,
   pfG6                    = 0xcb,
   pfzArchitectur          = 0xcc,
   pfIntelCi5              = 0xcd,
   pfIntelCi3              = 0xce,
   pfC7M                   = 0xd2,
   pfC7D                   = 0xd3,
   pfC7                    = 0xd4,
   pfEden                  = 0xd5,
   pfMCXeon                = 0xd6,
   pfDCXeon3xxx            = 0xd7,
   pfQCXeon3xxx            = 0xd8,
   pfDCXeon5xxx            = 0xda,
   pfQCXeon5xxx            = 0xdb,
   pfDCXeon7xxx            = 0xdd,
   pfQCXeon7xxx            = 0xde,
   pfMCXeon7xxx            = 0xdf,
   pfEmbeddedOpteronQC     = 0xe6,
   pfPhenomTC              = 0xe7,
   pfTurionUDCMobile       = 0xe8,
   pfTurionDCMobile        = 0xe9,
   pfAthlonDC              = 0xea,
   pfSempronSI             = 0xeb,
   pfPhenom_II             = 0xec,
   pfAthlon_II             = 0xed,
   pfOpteron_6C            = 0xee,
   pfSempron_M             = 0xef,
   pfi860                  = 0xfa,
   pfi960                  = 0xfb,
   pfUseProcFamily2Field   = 0xfe
};
enum Family2_t
{
   pfSH3                   = 0x0104,
   pfSH4                   = 0x0105,
   pfArm                   = 0x0118,
   pfStrongArm             = 0x0119,
   pf6x86                  = 0x012c,
   pfMediaGX               = 0x012d,
   pfMII                   = 0x012e,
   pfWinChip               = 0x0140,
   pfDSP                   = 0x015e,
   pfVideoProcessor        = 0x01f4,
};
enum CPUStatus_t
{
   csUnknown       =  0,
   csEnabled       =  1,
   csUserDisabled  =  2,
   csPostDisabled  =  3,
   csIdle          =  4,
   csOther         =  7
};
enum ProcUpgrade_t
{
   puOther                 = 0x01,
   puUnknown               = 0x02,
   puDaughterBoard         = 0x03,
   puZIFSocket             = 0x04,
   puReplaceablePiggyBack  = 0x05,
   puNone                  = 0x06,
   puLIFSocket             = 0x07,
   puSlot1                 = 0x08,
   puSlot2                 = 0x09,
   pu370PinSocket          = 0x0a,
   puSlotA                 = 0x0b,
   puSlotM                 = 0x0c,
   puSocket423             = 0x0d,
   puSocketA               = 0x0e,
   puSocket478             = 0x0f,
   puSocket754             = 0x10,
   puSocket940             = 0x11,
   puSocket939             = 0x12,
   puSocketmPGA604         = 0x13,
   puSocketLGA771          = 0x14,
   puSocketLGA775          = 0x15,
   puSocketS1              = 0x16,
   puSocketAM2             = 0x17,
   puSocketF               = 0x18,
   puSocketLGA1366         = 0x19,
   puSocketG34             = 0x1a,
   puSocketAM3             = 0x1b,
   puSocketC32             = 0x1c,
   puSocketLGA1156         = 0x1d,
   puSocketLGA1567         = 0x1e,
   puSocketPGA988A         = 0x1f,
   puSocketBGA1288         = 0x20,
   puSocketrPGA988B        = 0x21,
   puSocketBGA1023         = 0x22,
   puSocketBGA1224         = 0x23,
   puSocketBGA1155         = 0x24,
   puSocketLGA1356         = 0x25,
   puSocketLGA2011         = 0x26,
};
typedef struct _SMBIOS_PROCESSOR_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    bySocketDesignation;            /* string number */
   u_char    byProcessorType;                /* enumeration */
   u_char    byProcessorFamily;              /* enumeration */
   u_char    byProcessorManuf;               /* string number */
   u_int   dwProcessorID1;                 /* raw processor identification data (EAX) */
   u_int   dwProcessorID2;                 /* raw processor identification data (EDX) */
   u_char    byProcessorVersion;             /* string number */
   u_char    byVoltage;                      /* ??? */
   u_short    wExternalClockMHz;              /* external clock frequency */
   u_short    wMaxSpeed;                      /* maximum internal processor speed */
   u_short    wCurrentSpeed;                  /* current internal processor speed */
   /*u_char    byStatus; */                      /* ??? */
   u_char    bfCPUStatus           : 3;      /* Bits 0:2 */
   u_char    bfReserved1           : 3;      /* Bits 3:5 */
   u_char    bfCPUPopulated        : 1;      /* Bit  6 */
   u_char    bfReserved2           : 1;      /* Bit  7 */
   u_char    byProcessorUpgrade;             /* enumeration */
   u_short    wL1CacheHandle;                 /* cache information structure handle */
   u_short    wL2CacheHandle;                 /* cache information structure handle */
   u_short    wL3CacheHandle;                 /* cache information structure handle */
   u_char    bySerialNum;                    /* string number */
   u_char    byAssetTag;                     /* string number */
   u_char    byPartNum;                      /* string number */
   u_char    byCoreCount;                    /* core count (0==unknown) */
   u_char    byCoreEnabled;                  /* core enabled (0==unknown) */
   u_char    byThreadCount;                  /* Thread count (0==unknown) */
   /*u_short    wProcCharacteristics; */      /* ??? */
   u_short    bfReserved3           : 1;      /* Bit 0 */
   u_short    bfUnknown             : 1;      /* Bit 1 */
   u_short    bf64bitCapable        : 1;      /* Bit 2 */
   u_short    bfReserved4           : 13;     /* Bits 3:15 */
   u_short    wProcessorFamily2;              /* enumeration */
} SMBIOS_PROCESSOR_INFORMATION, *PSMBIOS_PROCESSOR_INFORMATION;

/* type 5 OBSOLETE */
typedef struct _SMBIOS_MEMORY_CONTROLLER_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byErrDetectMethod;              /* enum */
   u_char    byErrCorrectCapability;         /* enum */
   u_char    bySupportedInterleave;          /* enum */
   u_char    byCurrentInterleave;            /* enum */
   u_char    byMaxMemModuleSize;             /* largest memory module supported per slot, (n as in 2**n MB) */
   u_short    wSupportedSpeeds;               /* bit field */
   u_short    wSupportedMemoryTypes;          /* bit field */
   u_char    byMemoryModuleVoltage;          /* bit field */
   u_char    byNumberOfSlots;                /* how many memory module information blocks are associated with this controller */
   u_short    wMemModuleHandles[1];           /* this array can contain 0 to N handles in reality... */
} SMBIOS_MEMORY_CONTROLLER_INFORMATION, *PSMBIOS_MEMORY_CONTROLLER_INFORMATION;

/* type 6 OBSOLETE */
typedef struct _SMBIOS_MEMORY_MODULE_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    bySocketDesignation;            /* string */
   u_char    byBankConnections;              /* each nibble indicates a bank connection */
   u_char    byCurrentSpeed;                 /* The speed of the memory module in ns */
   u_short    wCurrentMemoryType;             /* bit field */
   u_char    byInstalledSize;                /* two fields, bit 7 and bits 6:0 */
   u_char    byEnabledSize;                  /* two fields, bit 7 and bits 6:0 */
   u_char    byErrorStatus;                  /* bit field */
} SMBIOS_MEMORY_MODULE_INFORMATION, *PSMBIOS_MEMORY_MODULE_INFORMATION;

/* type 7 */
enum CacheLevel_t
{
   clOther             =  1,
   clUnknown           =  2,
   clPrimary           =  3,     /* L1 */
   clSecondary         =  4,     /* L2 */
   clTertiary          =  5      /* L3 */
};
enum CacheLocation_t
{
   cloInternal         = 0,
   cloExternal         = 1,
   cloUnknown          = 3
};
enum CacheMode_t
{
   cmWriteThough       = 0,
   cmWriteBack         = 1,
   cmVaries            = 2,
   cmUnknown           = 3
};
enum CacheSRAMType_t
{
   cstOther                = 0x0001,
   cstUnknown              = 0x0002,
   cstNonBurst             = 0x0004,
   cstBurst                = 0x0008,
   cstPipelineBurst        = 0x0010,
   cstSynchronous          = 0x0020,
   cstAsynchronous         = 0x0040
};
enum CacheErrorType_t
{
   cetOther                 = 0x01,
   cetUnknown               = 0x02,
   cetNone                  = 0x03,
   cetParity                = 0x04,
   cetSingleBitECC          = 0x05,
   cetMultiBitECC           = 0x06
};
enum SystemCacheType_t
{
   sctOther                 = 0x01,
   sctUnknown               = 0x02,
   sctInstruction           = 0x03,
   sctData                  = 0x04,
   sctUnified               = 0x05
};
enum CacheAssociativity_t
{
   caOther                 = 0x01,
   caUnknown               = 0x02,
   caDirectMapped          = 0x03,
   ca2WaySetAssociative    = 0x04,
   ca4WaySetAssociative    = 0x05,
   caFullyAssociative      = 0x06,
   ca8WaySetAssociative    = 0x07,
   ca16WaySetAssociative   = 0x08,
   ca12WaySetAssociative   = 0x09,
   ca24WaySetAssociative   = 0x0a,
   ca32WaySetAssociative   = 0x0b,
   ca48WaySetAssociative   = 0x0c,
   ca64WaySetAssociative   = 0x0d,
   ca20WaySetAssociative   = 0x0e,
};
typedef struct _SMBIOS_CACHE_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    bySocketDesignation;            /* string number */
   /*u_short    wCacheConfig; */                  /* cache configuration */
   u_short    bfLevel        : 3;
   u_short    bfSocketed     : 1;
   u_short    bfReserved1    : 1;
   u_short    bfLocation     : 2;
   u_short    bfEnabled      : 1;
   u_short    bfMode         : 2;
   u_short    bfReserved2    : 6;
   /*u_short    wMaximumSize; */                  /* maximum cache size */
   u_short    bfMaximumSize        :15;
   u_short    bfMaximumSizeGrain   : 1;
   /*u_short    wInstalledSize; */                /* installed cache size */
   u_short    bfInstalledSize      :15;
   u_short    bfInstalledSizeGrain : 1;
   u_short    wSupportedSRAMType;             /* bit field */
   u_short    wCurrentSRAMType;               /* bit field */
   u_char    byCacheSpeed;                   /* cache module speed in ns */
   u_char    byECCType;                      /* error correction scheme */
   u_char    byCacheType;                    /* logical type of cache */
   u_char    byAssociativity;                /* cache associativity */
} SMBIOS_CACHE_INFORMATION, *PSMBIOS_CACHE_INFORMATION;

/* type 8 */
enum PortConnector_t
{
   pcNone               = 0x00,
   pcCentronics         = 0x01,
   pcMiniCentronics     = 0x02,
   pcProprietary        = 0x03,
   pcDB25Male           = 0x04,
   pcDB25Female         = 0x05,
   pcDB15Male           = 0x06,
   pcDB15Female         = 0x07,
   pcDB9Male            = 0x08,
   pcDB9Female          = 0x09,
   pcRJ11               = 0x0A,
   pcRJ45               = 0x0B,
   pc50PinMiniSCSI      = 0x0C,
   pcMiniDIN            = 0x0D,
   pcMicroDIN           = 0x0E,
   pcPS2                = 0x0F,
   pcInfrared           = 0x10,
   pcHPHIL              = 0x11,
   pcUSB                = 0x12,
   pcSSASCSI            = 0x13,
   pcCircularDIN8Male   = 0x14,
   pcCircularDIN8Female = 0x15,
   pcOnBoardIDE         = 0x16,
   pcOnBoardFloppy      = 0x17,
   pc9PinDualInline     = 0x18,
   pc25PinDualInline    = 0x19,
   pc50PinDualInline    = 0x1A,
   pc68PinDualInline    = 0x1B,
   pcOnBoardSoundCDROM  = 0x1C,
   pcMiniCentronics14   = 0x1D,
   pcMiniCentronics26   = 0x1E,
   pcMiniJack           = 0x1F,
   pcBNC                = 0x20,
   pc1394               = 0x21,
   pcSasSata            = 0x22,
   pcPC98               = 0xA0,
   pcPC98Hireso         = 0xA1,
   pcPCH98              = 0xA2,
   pcPC98Note           = 0xA3,
   pcPC98Full           = 0xA4,
   pcOther              = 0xFF
};
enum PortType_t
{
   pctNone               = 0x00,
   pctParallelXTAT       = 0x01,
   pctParallelPS2        = 0x02,
   pctParallelECP        = 0x03,
   pctParallelEPP        = 0x04,
   pctParallelECPEPP     = 0x05,
   pctSerialXTAT         = 0x06,
   pctSerial16450        = 0x07,
   pctSerial16550        = 0x08,
   pctSerial16550A       = 0x09,
   pctSCSIPort           = 0x0A,
   pctMIDIPort           = 0x0B,
   pctJoyStickPort       = 0x0C,
   pctKeyboardPort       = 0x0D,
   pctMousePort          = 0x0E,
   pctSSASCSI            = 0x0F,
   pctUSB                = 0x10,
   pctFireWire           = 0x11,
   pctPCMCIAType1        = 0x12,
   pctPCMCIAType2        = 0x13,
   pctPCMCIAType3        = 0x14,
   pctCardBus            = 0x15,
   pctAccessBusPort      = 0x16,
   pctSCSI2              = 0x17,
   pctSCSIWide           = 0x18,
   pctPC98               = 0x19,
   pctPC98Hireso         = 0x1A,
   pctPCH98              = 0x1B,
   pctVideoPort          = 0x1C,
   pctAudioPort          = 0x1D,
   pctModemPort          = 0x1E,
   pctNetworkPort        = 0x1F,
   pctSATA               = 0x20,
   pctSAS                = 0x21,
   pct8251Compatible     = 0xA0,
   pct8251FIFOCompatible = 0xA1,
   pctOther              = 0xFF
};
typedef struct _SMBIOS_PORT_CONNECTOR_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byInternalRefDesig;             /* string */
   u_char    byInternalConnType;             /* enum */
   u_char    byExternalRefDesig;             /* string */
   u_char    byExternalConnType;             /* enum */
   u_char    byPortType;                     /* enum */
} SMBIOS_PORT_CONNECTOR_INFORMATION, *PSMBIOS_PORT_CONNECTOR_INFORMATION;

/* type 9 */
enum SlotType_t
{
   sltOther            = 0x01,
   sltUnknown          = 0x02,
   sltISA              = 0x03,
   sltMCA              = 0x04,
   sltEISA             = 0x05,
   sltPCI              = 0x06,
   sltPCCard           = 0x07,
   sltVLVESA           = 0x08,
   sltProprietary      = 0x09,
   sltProcessor        = 0x0A,
   sltProprietaryMem   = 0x0B,
   sltIORiserSlot      = 0x0C,
   sltNUBUS            = 0x0D,
   sltPCI66            = 0x0E,
   sltAGP              = 0x0F,
   sltAGP2x            = 0x10,
   sltAGP4x            = 0x11,
   sltPCIX             = 0x12,
   sltAGP8x            = 0x13,
   sltPC98C20          = 0xA0,
   sltPC98C24          = 0xA1,
   sltPC98E            = 0xA2,
   sltPC98LocalBus     = 0xA3,
   sltPC98Card         = 0xA4,
   sltPCIE             = 0xA5,
   sltPCIExpressx1     = 0xA6,
   sltPCIExpressx2     = 0xA7,
   sltPCIExpressx4     = 0xA8,
   sltPCIExpressx8     = 0xA9,
   sltPCIExpressx16    = 0xAA,
   sltPCIExpressG2     = 0xAB,
   sltPCIExpressG2x1   = 0xAC,
   sltPCIExpressG2x2   = 0xAD,
   sltPCIExpressG2x4   = 0xAE,
   sltPCIExpressG2x8   = 0xAF,
   sltPCIExpressG2x16  = 0xB0
};
enum SlotWidth_t
{
   slwOther            = 0x01,
   slwUnknown          = 0x02,
   slw8bit             = 0x03,
   slw16bit            = 0x04,
   slw32bit            = 0x05,
   slw64bit            = 0x06,
   slw128bit           = 0x07,
   slwx1               = 0x08,
   slwx2               = 0x09,
   slwx4               = 0x0A,
   slwx8               = 0x0B,
   slwx12              = 0x0C,
   slwx16              = 0x0D,
   slwx32              = 0x0E
};
enum SlotUsage_t
{
   sluOther            = 0x01,
   sluUnknown          = 0x02,
   sluAvailable        = 0x03,
   sluInUse            = 0x04
};
enum SlotLength_t
{
   sllOther            = 0x01,
   sllUnknown          = 0x02,
   sllShort            = 0x03,
   sllLong             = 0x04
};
enum SlotCharacteristics1_t
{
   slc1Unknown         = 0x01,
   slc1Volts50         = 0x02,
   slc1Volts33         = 0x04,
   slc1Shared          = 0x08,
   slc1PCCard16        = 0x10,
   slc1PCCardCardBus   = 0x20,
   slc1PCCardZoom      = 0x40,
   slc1PCCardModemRing = 0x80
};
enum SlotCharacteristics2_t
{
   slc2PME             = 0x01,
   slc2HotPlug         = 0x02,
   slcSMBusSignal      = 0x03
};
typedef struct _SMBIOS_SYSTEM_SLOTS
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    bySlotDesignation;              /* string, reference designation */
   u_char    bySlotType;                     /* enum */
   u_char    bySlotDataBusWidth;             /* enum */
   u_char    byCurrentUsage;                 /* enum */
   u_char    bySlotLength;                   /* enum */
   u_short    wSlotID;                        /* see slot type */
   u_char    bySlotCharacteristics1;         /* bit field */
   u_char    bySlotCharacteristics2;         /* bit field */
   u_short    wSegment;
   u_char    byBusNumber;
   u_char    byFunctionNumber      : 3;
   u_char    byDeviceNumber        : 5;
} SMBIOS_SYSTEM_SLOTS, *PSMBIOS_SYSTEM_SLOTS;

/* type 10 OBSOLETE */
enum DeviceType_t
{
   dvtOther            = 0x01,
   dvtUnknown          = 0x02,
   dvtVideo            = 0x03,
   dvtSCSI             = 0x04,
   dvtEthernet         = 0x05,
   dvtTokenRing        = 0x06,
   dvtSound            = 0x07,
   dvtPATA             = 0x08,
   dvtSATA             = 0x09,
   dvtSAS              = 0x0A,
};
typedef struct _SMBIOS_ONBOARD_DEVICES_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byData[1];                      /* variable length array */
} SMBIOS_ONBOARD_DEVICES_INFORMATION, *PSMBIOS_ONBOARD_DEVICES_INFORMATION;

/* type 11 */
typedef struct _SMBIOS_OEM_STRINGS_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byCount;                        /* number of strings */
} SMBIOS_OEM_STRINGS_INFORMATION, *PSMBIOS_OEM_STRINGS_INFORMATION;

/* type 12 */
typedef struct _SMBIOS_SYSTEM_CONFIGURATION_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byCount;                        /* number of strings */
} SMBIOS_SYSTEM_CONFIGURATION_INFORMATION, *PSMBIOS_SYSTEM_CONFIGURATION_INFORMATION;

/* type 13 */
typedef struct _SMBIOS_BIOS_LANGUAGE_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byInstallableLanguages;         /* number of languages available */
   u_char    byFlags;                        /* bit field */
   u_char    byReserved[15];                 /* reserved */
   u_char    byCurrentLanguage;              /* string number of the currently installed language */
} SMBIOS_BIOS_LANGUAGE_INFORMATION, *PSMBIOS_BIOS_LANGUAGE_INFORMATION;

/* type 14 */
typedef struct _SMBIOS_GRP_ASSOCIATIONS
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byName;                         /* string number of group name */
   struct
   {
      u_char    byType;                      /* Structure type */
      u_short    wHandle;                     /* Structure handle */
   } item[1];
} SMBIOS_GRP_ASSOCIATIONS, *PSMBIOS_GRP_ASSOCIATIONS;

/* type 15 */
typedef struct _SMBIOS_BIOS_SYS_EVENT_LOG
{
   SMBIOS_HEADER Header;                   /* common header */
   u_short    wLogAreaLength;                 /* length of the log area */
   u_short    wLogHeaderOffset;               /* Offset of log header from access method addr. */
   u_short    wLogDataOffset;                 /* Offset of log data from access method addr. */
   u_char    byAccessMethod;                 /* Data access method */
   u_char    bfLogAreaValid  :1;             /* Log area valid */
   u_char    bfLogAreaFull   :1;             /* Log area full */
   u_char    reserved        :6;             /* reserved */
   u_char    byLogChangedToken;              /* Changed when new events added */
   u_int   dwAccessMethodAddr;             /* Access method address */
   u_char    byLogHdrFormat;                 /* Log Header format */
   u_char    byNumDescriptors;               /* Number of log type desciptors supported */
   u_char    byLenDescriptors;               /* Length of each log type desciptor */
   struct
   {
      u_char    byLogType;                   /* Structure type */
      u_char    byDataFormat;                /* Structure handle */
   } desc[1];
} SMBIOS_BIOS_SYS_EVENT_LOG, *PSMBIOS_BIOS_SYS_EVENT_LOG;

/* type 16 */
enum MemeryArrayLoc_t
{
   malOther             = 0x01,
   malUnknown           = 0x02,
   malSystemboard       = 0x03,
   malISABoard          = 0x04,
   malEISABoard         = 0x05,
   malPCIBoard          = 0x06,
   malMCABoard          = 0x07,
   malPCMCIABoard       = 0x08,
   malProprietary       = 0x09,
   malNuBus             = 0x0A,
   malPC98C20           = 0xA0,
   malPC98C24           = 0xA1,
   malPC98E             = 0xA2,
   malPC98LocalBus      = 0xA3
};
enum MemeryArrayUse_t
{
   mauOther             = 0x01,
   mauUnknown           = 0x02,
   mauSystem            = 0x03,
   mauVideo             = 0x04,
   mauFlash             = 0x05,
   mauNVRAM             = 0x06,
   mauCache             = 0x07
};
enum MemeryArrayECCType_t
{
   maeOther             = 0x01,
   maeUnknown           = 0x02,
   maeNone              = 0x03,
   maeParity            = 0x04,
   maeSingleBitECC      = 0x05,
   maeMultiBitECC       = 0x06,
   maeCRC               = 0x07
};
typedef struct _SMBIOS_PHYSICAL_MEMORY_ARRAY
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byLocation;                     /* enum */
   u_char    byUse;                          /* enum */
   u_char    byErrorCorrection;              /* enum */
   u_int   dwMaximumCapacity;              /* the maximum memory capacity in KB */
   u_short    wErrorInfoHandle;               /* the handle associated with any error */
   u_short    wNumberOfDevices;               /* the number of sockets available for memory devices */
} SMBIOS_PHYSICAL_MEMORY_ARRAY, *PSMBIOS_PHYSICAL_MEMORY_ARRAY;

/* type 17 */
enum MemeryDeviceFormFactor_t
{
   mdffOther             = 0x01,
   mdffUnknown           = 0x02,
   mdffSIMM              = 0x03,
   mdffSIP               = 0x04,
   mdffChip              = 0x05,
   mdffDIP               = 0x06,
   mdffZIP               = 0x07,
   mdffProprietaryCard   = 0x08,
   mdffDIMM              = 0x09,
   mdffTSOP              = 0x0A,
   mdffRowOfChips        = 0x0B,
   mdffRIMM              = 0x0C,
   mdffSODIMM            = 0x0D,
   mdffSRIMM             = 0x0E,
   mdffFBDIMM            = 0x0F
};
enum MemeryDeviceType_t
{
   memdtOther             = 0x01,
   memdtUnknown           = 0x02,
   memdtDRAM              = 0x03,
   memdtEDRAM             = 0x04,
   memdtVRAM              = 0x05,
   memdtSRAM              = 0x06,
   memdtRAM               = 0x07,
   memdtROM               = 0x08,
   memdtFLASH             = 0x09,
   memdtEEPROM            = 0x0A,
   memdtFEPROM            = 0x0B,
   memdtEPROM             = 0x0C,
   memdtCDRAM             = 0x0D,
   memdt3DRAM             = 0x0E,
   memdtSDRAM             = 0x0F,
   memdtSGRAM             = 0x10,
   memdtRDRAM             = 0x11,
   memdtDDR               = 0x12,
   memdtDDR2              = 0x13,
   memdtDDR2FBDIMM        = 0x14,
   memdtReserve1          = 0x15,
   memdtReserve2          = 0x16,
   memdtReserve3          = 0x17,
   memdtDDR3              = 0x18,
   memdtFBD2              = 0x19
};
enum MemeryDeviceTypeDetail_t
{
   mdtdReserved          = 0x0001,
   mdtdOther             = 0x0002,
   mdtdUnknown           = 0x0004,
   mdtdFastPaged         = 0x0008,
   mdtdStaticColumn      = 0x0010,
   mdtdPseudoStatic      = 0x0020,
   mdtdRAMBUS            = 0x0040,
   mdtdSynchronous       = 0x0080,
   mdtdCMOS              = 0x0100,
   mdtdEDO               = 0x0200,
   mdtdWindowDRAM        = 0x0400,
   mdtdCacheDRAM         = 0x0800,
   mdtdNonVolatile       = 0x1000
};
typedef struct _SMBIOS_MEMORY_DEVICE
{
   SMBIOS_HEADER Header;                   /* common header */
   u_short    wArrayHandle;                   /* the handle of the array which this device belongs to */
   u_short    wErrorInfoHandle;               /* the handle associated with any error */
   u_short    wTotalWidth;                    /* the total width of this memory device, including any ECC bits */
   u_short    wDataWidth;                     /* the data width of this memory device */
   /*u_short    wSize; */                         /* the size of the memory device */
   u_short    bfSize         :15;
   u_short    bfKBytesSized  : 1;
   u_char    byFormFactor;                   /* enum */
   u_char    byDeviceSet;                    /* is this device part of a set? */
   u_char    byDeviceLocator;                /* string */
   u_char    byBankLocator;                  /* string */
   u_char    byMemoryType;                   /* enum */
   u_short    wTypeDetail;                    /* bit field */
   u_short    wSpeed;                         /* Speed in MHz */
   u_char    byManufacturer;                 /* string */
   u_char    bySerialNum;                    /* string */
   u_char    byAssetTag;                     /* string */
   u_char    byPartNum;                      /* string */
   /*u_char    byAttritures; */              /* the attributes of the memory device */
   u_char    bfRank         : 4;             /* 0==unknown */
   u_char    bfReserved     : 4;
   u_int   dwExtendedSize;                 /* The extended size of the memory device */
   u_short    wMemoryClockSpeed;              /* clock speed to the memory device, in megahertz (MHz)  */
} SMBIOS_MEMORY_DEVICE, *PSMBIOS_MEMORY_DEVICE;

/* type 18 */
enum MemeryErrorType_t
{
   merrtOther             = 0x01,
   merrtUnknown           = 0x02,
   merrtOK                = 0x03,
   merrtBadRead           = 0x04,
   merrtParityError       = 0x05,
   merrtSingleBit         = 0x06,
   merrtDoubleBit         = 0x07,
   merrtMultiBit          = 0x08,
   merrtNibble            = 0x09,
   merrtChecksum          = 0x0A,
   merrtCRC               = 0x0B,
   merrtCorrectedSingleBit= 0x0C,
   merrtCorrected         = 0x0D,
   merrtUncorrectable     = 0x0E
};
enum MemeryErrorGranularity_t
{
   merrgOther             = 0x01,
   merrgUnknown           = 0x02,
   merrgDevice            = 0x03,
   merrgMemoryPartition   = 0x04
};
enum MemeryErrorOperation_t
{
   merroOther             = 0x01,
   merroUnknown           = 0x02,
   merroRead              = 0x03,
   merroWrite             = 0x04,
   merroPartialWrite      = 0x05
};
typedef struct _SMBIOS_MEMORY_ERROR_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byErrorType;                    /* enum */
   u_char    byErrorGranularity;             /* enum */
   u_char    byErrorOperation;               /* enum */
   u_int   dwVendorSyndrome;               /* the vendor-specific ECC syndrome */
   u_int   dwArrayErrorAddress;            /* the 32-bit physical address of the error */
   u_int   dwDeviceErrorAddress;           /* the 32-bit physical address of the error */
   u_int   dwErrorResolution;              /* the range, in bytes, within which the error can be determined */
} SMBIOS_MEMORY_ERROR_INFORMATION, *PSMBIOS_MEMORY_ERROR_INFORMATION;

/* type 19 */
typedef struct _SMBIOS_MEMORY_ARRAY_MAPPED_ADDRESS
{
   SMBIOS_HEADER Header;                   /* common header */
   u_int   dwStartingAddress;              /* the physical address, in KB, of a memory range mapped to the specified physical memory array */
   u_int   dwEndingAddress;                /* the physical ending address of the last KB of a memory range mapped to the specified physical memory array */
   u_short    wMemoryArrayHandle;             /* the handle of the associated physical memory array */
   u_char    byPartitionWidth;               /* the number of memory devices that form a single row of memory for the address partition */
//   u_int   dwExtStartingAddress;           /* the physical address, in bytes, of a ramge of memory mapped to the specified physical memory array */
//   u_int   dwExtEndingAddress;             /* the physical ending address in bytes, of a range of memory mapped to the specified physical memory array */
} SMBIOS_MEMORY_ARRAY_MAPPED_ADDRESS, *PSMBIOS_MEMORY_ARRAY_MAPPED_ADDRESS;

/* type 20 */
typedef struct _SMBIOS_MEMORY_DEVICE_MAPPED_ADDRESS
{
   SMBIOS_HEADER Header;                   /* common header */
   u_int   dwStartingAddress;              /* the physical address, in KB, of a memory range mapped to the referenced memory device */
   u_int   dwEndingAddress;                /* the physical ending address of the last KB of an address range mapped to the referenced memory device */
   u_short    wMemoryDeviceHandle;            /* the handle of the associated memory device structure */
   u_short    wMappedAddressHandle;           /* the handle associated with the memory array mapped address structure */
   u_char    byPartitionRowPosition;         /* the position of the referenced memory device */
   u_char    byInterleavePosition;           /* the position of the referenced memory device in an interleave */
   u_char    byInterleavedDataDepth;         /* the number of rows from the referenced memory device accessed in a single interleaved transfer */
//   u_int   dwExtStartingAddress;           /* the physical address, in bytes, of a ramge of memory mapped to the specified physical memory array */
//   u_int   dwExtEndingAddress;             /* the physical ending address in bytes, of a range of memory mapped to the specified physical memory array */
} SMBIOS_MEMORY_DEVICE_MAPPED_ADDRESS, *PSMBIOS_MEMORY_DEVICE_MAPPED_ADDRESS;

/* type 21 */
enum PointingDeviceType_t
{
   pdtOther               = 0x01,
   pdtUnknown             = 0x02,
   pdtMouse               = 0x03,
   pdtTrackBall           = 0x04,
   pdtTrackPoint          = 0x05,
   pdtGlidePoint          = 0x06,
   pdtTouchPad            = 0x07,
   pdtTouchScreen         = 0x08,
   pdtOpticalSensor       = 0x09,
};
enum PointingDeviceInterface_t
{
   pdiOther               = 0x01,
   pdiUnknown             = 0x02,
   pdiSerial              = 0x03,
   pdiPS2                 = 0x04,
   pdiInfrared            = 0x05,
   pdiHPHIL               = 0x06,
   pdiBusMouse            = 0x07,
   pdiADB                 = 0x08,
   pdiBusMouseDB9         = 0xA0,
   pdiBusMouseMicroDIN    = 0xA1,
   pdiUSB                 = 0xA2,
};
typedef struct _SMBIOS_BUILTIN_POINTNG_DEVICE
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byType;                         /* enum of the pointing device type */
   u_char    byInterface;                    /* enum of interface type */
   u_char    byNumButtons;                   /* the number of buttons */
} SMBIOS_BUILTIN_POINTNG_DEVICE, *PSMBIOS_BUILTIN_POINTNG_DEVICE;

/* type 22 */
typedef struct _SMBIOS_PORTABLE_BATT
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byLocation;                     /* string of location */
   u_char    byManufacturer;                 /* string of manufacturer name */
   u_char    byManufactureDate;              /* string of manufacturer date */
   u_char    bySerialNum;                    /* string of serial number */
   u_char    byDeviceName;                   /* string of device name */
   u_char    byDeviceChemistry;              /* enum of chemistry type */
   u_short    wDesignCapacity;                /* design capacity in mWatt-hours */
   u_short    wDesignVoltage;                 /* design voltage in mVolts */
   u_char    bySBDSVersion;                  /* string of Smart Battery Data Spec version */
   u_char    byMaxErrorInData;               /* max error as % in mwatt-hour data */
   u_short    wSBDSSerialNum;                 /* design voltage in mVolts */
   u_short    bfSBDSManfDay    :5;            /* SDBS Manufacturer's date day */
   u_short    bfSBDSManfMonth  :4;            /* SDBS Manufacturer's date month */
   u_short    bfSBDSManfYear   :7;            /* SDBS Manufacturer's date year 1980 baised */
   u_char    bySBDSChemistry;                /* SDBS chemistry string */
   u_char    byCapacityMultiplier;           /* Capacity multiplier */
   u_int   dwOEMSpecific;                  /* OEM or BIOS specific data */
} SMBIOS_PORTABLE_BATT, *PSMBIOS_PORTABLE_BATT;

/* type 23 */
enum BootOption_t
{
   boReserved      =  0,
   boOS            =  1,
   boSysUtilities  =  2,
   boDoNotBoot     =  3
};
typedef struct _SMBIOS_SYSTEM_RESET
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    bfStatus        :1;             /* System reset enabled or not */
   u_char    bfBootOption    :2;             /* action to take on watchdog reset */
   u_char    bfBootOnLimit   :2;             /* action to take on if limit reached */
   u_char    bfWatchdog      :1;             /* system contains a watchdog timer */
   u_char    bfReserved      :2;             /* reserved */
   u_char    wResetCount;                    /* resets since last user initiated reset */
   u_char    wResetLimit;                    /* # of times reset will be attempted */
   u_char    wTimerInterval;                 /* minutes for watchdog timer */
   u_char    wTimeout;                       /* reboot timeout in minutes */
} SMBIOS_SYSTEM_RESET, *PSMBIOS_SYSTEM_RESET;

/* type 24 */
enum HardwareSecurityStatus_t
{
   hwsDisabled      =  0,
   hwsEnabled       =  1,
   hwsNotImplemented=  2,
   hwsUnknown       =  3
};
typedef struct _SMBIOS_HW_SECURITY
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    bfFrontPanelReset  :2;          /* front panel reset status */
   u_char    bfAdminPW          :2;          /* administrator password status */
   u_char    bfKeyboardPW       :2;          /* keyboard password status */
   u_char    bfPowerOnPW        :2;          /* power-on password status */
} SMBIOS_HW_SECURITY, *PSMBIOS_HW_SECURITY;

/* type 25 */
typedef struct _SMBIOS_SYS_POWER_CONTROLS
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byPwrOnMonth;                   /* next scheduled power on month (in BCD) */
   u_char    byPwrOnDay;                     /* next scheduled power on day (in BCD) */
   u_char    byPwrOnHour;                    /* next scheduled power on hour (in BCD) */
   u_char    byPwrOnMinute;                  /* next scheduled power on minute (in BCD) */
   u_char    byPwrOnSecond;                  /* next scheduled power on second (in BCD) */
} SMBIOS_SYS_POWER_CONTROLS, *PSMBIOS_SYS_POWER_CONTROLS;

/* type 26 */
enum VoltageProbeLocation_t
{
   vplOther             =   1,
   vplUnknown           =   2,
   vplProcessor         =   3,
   vplDisk              =   4,
   vplPeripheralBay     =   5,
   vplSysMgtModule      =   6,
   vplMotherboard       =   7,
   vplMemoryModule      =   8,
   vplProcModule        =   9,
   vplPowerUnit         =  10,
   vplAddInCard         =  11,
};
enum VoltageProbeStatus_t
{
   vpsOther          =  1,
   vpsUnknown        =  2,
   vpsOK             =  3,
   vpsNonCritical    =  4,
   vpsCritical       =  5,
   vpsNonRecoverable =  6
};
typedef struct _SMBIOS_VOLTAGE_PROBE_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byDescription;                  /* string of descriptive info */
   u_char    bfLocation      :5;             /* enum of location */
   u_char    bfStatus        :3;             /* enum of status */
   u_short    wMaxValue;                      /* Max voltage level readable by this probe in mVolts */
   u_short    wMinValue;                      /* Min voltage level readable by this probe in mVolts */
   u_short    wResolution;                    /* resolution in tenth's of mVolts */
   u_short    wTolerance;                     /* tolerance in +/- mVolts */
   u_short    wAccuracy;                      /* accuracy in +/- 1/100th percent units */
   u_int   dwOEMDefined;                   /* OEM defined field */
   u_short    wNominalValue;                  /* nominal value in mVolts */
} SMBIOS_VOLTAGE_PROBE_INFO, *PSMBIOS_VOLTAGE_PROBE_INFO;

/* type 27 */
enum CoolingDeviceType_t
{
   cdtOther         =   1,
   cdtUnknown       =   2,
   cdtFan           =   3,
   cdtBlower        =   4,
   cdtChipFan       =   5,
   cdtCabinetFan    =   6,
   cdtPowerSupplyFan=   7,
   cdtHeatPipe      =   8,
   cdtIntRefrig     =   9,
   cdtActiveCooling =  10,
   cdtPassiceCooling=  11
};
enum CoolingDeviceeStatus_t
{
   cdsOther          =  1,
   cdsUnknown        =  2,
   cdsOK             =  3,
   cdsNonCritical    =  4,
   cdsCritical       =  5,
   cdsNonRecoverable =  6
};
typedef struct _SMBIOS_COOLING_DEVICE_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_short    wTemperatureProbe;              /* temerature probe handle */
   u_char    bfDeviceType    :5;             /* enum of device type */
   u_char    bfStatus        :3;             /* enum of status */
   u_char    byCoolingGroup;                 /* cooling unit group (o==no group) */
   u_int   dwOEMDefined;                   /* OEM defined field */
   u_short    wNominalSpeed;                  /* nominal speed in rpm's */
} SMBIOS_COOLING_DEVICE_INFO, *PSMBIOS_COOLING_DEVICE_INFO;

/* type 28 */
enum TemperatureProbeLocation_t
{
   tplOther             =   1,
   tplUnknown           =   2,
   tplProcessor         =   3,
   tplDisk              =   4,
   tplPeripheralBay     =   5,
   tplSysMgtModule      =   6,
   tplMotherboard       =   7,
   tplMemoryModule      =   8,
   tplProcModule        =   9,
   tplPowerUnit         =  10,
   tplAddInCard         =  11,
   tplFrontPanelBoard   =  12,
   tplBackPanelBoard    =  13,
   tplPowerSystemBoard  =  14,
   tplDriveBackPlane    =  15,
};
enum TemperatureProbeStatus_t
{
   tpsOther          =  1,
   tpsUnknown        =  2,
   tpsOK             =  3,
   tpsNonCritical    =  4,
   tpsCritical       =  5,
   tpsNonRecoverable =  6
};
typedef struct _SMBIOS_TEMPERATURE_PROBE_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byDescription;                  /* string of descriptive info */
   u_char    bfLocation      :5;             /* enum of location */
   u_char    bfStatus        :3;             /* enum of status */
   u_short    wMaxValue;                      /* Max temp level readable by this probe in 1/10 degrees C */
   u_short    wMinValue;                      /* Min temp level readable by this probe in 1/10 degrees C */
   u_short    wResolution;                    /* resolution in 1/1000 degrees C */
   u_short    wTolerance;                     /* tolerance in +/- 1/10 degrees C */
   u_short    wAccuracy;                      /* accuracy in +/- 1/100th percent units */
   u_int   dwOEMDefined;                   /* OEM defined field */
   u_short    wNominalValue;                  /* nominal value in 1/10 degrees C */
} SMBIOS_TEMPERATURE_PROBE_INFO, *PSMBIOS_TEMPERATURE_PROBE_INFO;

/* type 29 */
enum ElecCurrentProbeLocation_t
{
   eplOther         =   1,
   eplUnknown       =   2,
   eplProcessor     =   3,
   eplDisk          =   4,
   eplPeripheralBay =   5,
   eplSysMgtModule  =   6,
   eplMotherboard   =   7,
   eplMemoryModule  =   8,
   eplProcModule    =   9,
   eplPowerUnit     =  10,
   eplAddInCard     =  11
};
enum ElecCurrentProbeStatus_t
{
   epsOther          =  1,
   epsUnknown        =  2,
   epsOK             =  3,
   epsNonCritical    =  4,
   epsCritical       =  5,
   epsNonRecoverable =  6
};
typedef struct _SMBIOS_ELEC_CURRENT_PROBE_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byDescription;                  /* string of descriptive info */
   u_char    bfLocation      :5;             /* enum of location */
   u_char    bfStatus        :3;             /* enum of status */
   u_short    wMaxValue;                      /* Max temp level readable by this probe in 1/10 degrees C */
   u_short    wMinValue;                      /* Min temp level readable by this probe in 1/10 degrees C */
   u_short    wResolution;                    /* resolution in 1/1000 degrees C */
   u_short    wTolerance;                     /* tolerance in +/- 1/10 degrees C */
   u_short    wAccuracy;                      /* accuracy in +/- 1/100th percent units */
   u_int   dwOEMDefined;                   /* OEM defined field */
   u_short    wNominalValue;                  /* nominal value in 1/10 degrees C */
} SMBIOS_ELEC_CURRENT_PROBE_INFO, *PSMBIOS_ELEC_CURRENT_PROBE_INFO;

/* type 30 */
typedef struct _SMBIOS_OUT_OF_BAND_REMOTE_ACCESS
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byManufacturerName;             /* string of manufacturer name */
   u_char    bfInboundEnabled    :1;         /* inbound connection enabled */
   u_char    bfOutboundEnabled   :1;         /* outbound connection enabled */
   u_char    bfReserved          :6;         /* reserved */
} SMBIOS_OUT_OF_BAND_REMOTE_ACCESS, *PSMBIOS_OUT_OF_BAND_REMOTE_ACCESS;

/* type 31 BIS ??? */

/* type 32 */
typedef struct _SMBIOS_SYS_BOOT_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byReserved[6];                  /* reserved area */
   u_char    byStatus[1];                    /* boot status (at least 1 byte) */
} SMBIOS_SYS_BOOT_INFO, *PSMBIOS_SYS_BOOT_INFO;

/* type 33 */
typedef struct _SMBIOS_64BIT_MEM_ERR_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byErrorType;                    /* enum */
   u_char    byErrorGranularity;             /* enum */
   u_char    byErrorOperation;               /* enum */
   u_int   dwVendorSyndrome;               /* the vendor-specific ECC syndrome */
   u_int   dwArrayErrorAddressLo;          /* the lower 32-bits of physical address of the error */
   u_int   dwArrayErrorAddressHi;          /* the upper 32-bits of physical address of the error */
   u_int   dwDeviceErrorAddressLo;         /* the lower 32-bits of physical address of the error */
   u_int   dwDeviceErrorAddressHi;         /* the upper 32-bits of physical address of the error */
   u_int   dwErrorResolution;              /* the range, in bytes, within which the error can be determined */
} SMBIOS_64BIT_MEM_ERR_INFO, *PSMBIOS_64BIT_MEM_ERR_INFO;

/* type 34 */
enum MgmtDeviceType_t
{
   mdtOther          =  0x01,
   mdtUnknown        =  0x02,
   mdtLM75           =  0x03,
   mdtLM78           =  0x04,
   mdtLM79           =  0x05,
   mdtLM80           =  0x06,
   mdtLM81           =  0x07,
   mdtADM9240        =  0x08,
   mdtDS1780         =  0x09,
   mdtMaxim1617      =  0x0A,
   mdtGL518SM        =  0x0B,
   mdtW83781D        =  0x0C,
   mdtHT82H791       =  0x0D
};
enum MgmtDeviceAddrType_t
{
   mdaOther          =  0x01,
   mdaUnknown        =  0x02,
   mdaIOPort         =  0x03,
   mdaMemory         =  0x04,
   mdaSMBus          =  0x05
};
typedef struct _SMBIOS_MGMT_DEVICE
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byDescription;                  /* string of description */
   u_char    byDeviceType;                   /* enum of device type */
   u_int   dwDeviceAddr;                   /* device address */
   u_char    byAddrType;                     /* address type */
} SMBIOS_MGMT_DEVICE, *PSMBIOS_MGMT_DEVICE;

/* type 35 */
typedef struct _SMBIOS_MGMT_DEVICE_COMPONENT
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byDescription;                  /* string of description */
   u_short    wMgmtDeviceHandle;              /* handle of management device */
   u_short    wComponentHandle;               /* handle of component device (temp, elec, cooling, etc.) */
   u_short    wThresholdHandle;               /* handle of device threshold */
} SMBIOS_MGMT_DEVICE_COMPONENT, *PSMBIOS_MGMT_DEVICE_COMPONENT;

/* type 36 */
typedef struct _SMBIOS_MGMT_DEVICE_THRESHOLD
{
   SMBIOS_HEADER Header;                   /* common header */
   u_short    wLowerThrshNonCritical;         /* lower threshold - non-critical */
   u_short    wUpperThrshNonCritical;         /* upper threshold - non-critical */
   u_short    wLowerThrshCritical;            /* lower threshold - critical */
   u_short    wUpperThrshCritical;            /* upper threshold - critical */
   u_short    wLowerThrshNonRecov;            /* lower threshold - non-recoverable */
   u_short    wUpperThrshNonRecov;            /* upper threshold - non-recoverable */
} SMBIOS_MGMT_DEVICE_THRESHOLD, *PSMBIOS_MGMT_DEVICE_THRESHOLD;

/* type 37 */
enum MemoryChannelType_t
{
   mctOther          =  0x01,
   mctUnknown        =  0x02,
   mctRamBus         =  0x03,
   mctSyncLink       =  0x04
};
typedef struct _SMBIOS_MEMORY_CHANNEL
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byChannelType;                  /* enum - memory type for channel */
   u_char    byMaxChannelLoad;               /* max load supported by the channel */
   u_char    byMemoryDeviceCount;            /* number of following structures */
   struct
   {
      u_char    byMemoryDeviceLoad;          /* Nth memory device load */
      u_short    wMemoryDeviceHandle;         /* Nth memory Device handle */
   } md[1];
} SMBIOS_MEMORY_CHANNEL, *PSMBIOS_MEMORY_CHANNEL;

/* type 38 */
enum IpmiBmcType_t
{
   bmctUnknown        =  0x00,
   bmctKCS            =  0x01,
   bmctSMIC           =  0x02,
   bmctBT             =  0x03
};
typedef struct _SMBIOS_IPMI_DEVICE
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byInterfaceType;                /* enum - interface type */
   u_char    byIPMISpecRevBDCLo  : 4;        /* IPMI spec rev. */
   u_char    byIPMISpecRevBDCHi  : 4;        /* IPMI spec rev. */
   u_char    byI2CSlaveAddress;              /* IIC Slave address */
   u_char    byNVStorageDevAddr;             /* Bus ID of NV storage device (0xFF if none) */
   u_int   dwBaseAddressLo;                /* the lower 32-bits of base address */
   u_int   dwBaseAddressHi;                /* the higher 32-bits of base address */
} SMBIOS_IPMI_DEVICE, *PSMBIOS_IPMI_DEVICE;

/* type 39 */
enum PowerSupplyType_t
{
   pstOther          =  0x01,
   pstUnknown        =  0x02,
   pstLinear         =  0x03,
   pstSwitching      =  0x04,
   pstBattery        =  0x05,
   pstUPS            =  0x06,
   pstConverter      =  0x07,
   pstRegulator      =  0x08
};
enum PowerSupplyStatus_t
{
   psstOther         =  0x01,
   psstUnknown       =  0x02,
   psstOK            =  0x03,
   psstNonCritical   =  0x04,
   psstCritical      =  0x05,
};
enum PowerSupplyInputVoltageRangeSwitching_t
{
   psivOther         =  0x01,
   psivUnknown       =  0x02,
   psivManual        =  0x03,
   psivAutoSwitch    =  0x04,
   psivWideRange     =  0x05,
   psivNotApplicable =  0x06
};
typedef struct _SMBIOS_SYSTEM_POWER_SUPPLY
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byPowerUnitGroup;               /* power suply group number */
   u_char    byLocation;                     /* string - location */
   u_char    byDeviceName;                   /* string - device name */
   u_char    byManufacturer;                 /* string - Manufacturer */
   u_char    bySerialNum;                    /* string - Serial number */
   u_char    byAssetTag;                     /* string - Asset tag */
   u_char    byModelPartNum;                 /* string - Model part number */
   u_char    byRevisionLevel;                /* string - Revision level */
   u_short    wMaxPowerCapacity;              /* Max sustained power output in watts (0x8000 if unknown) */
   u_short    wHotReplaceable       : 1;      /* Hot Replaceable ? */
   u_short    wPresent              : 1;      /* Present ? */
   u_short    wUnplugged            : 1;      /* Unplugged from wall ? */
   u_short    wInputVoltageRange    : 4;      /* enum - see above */
   u_short    wStatus               : 3;      /* enum - see above */
   u_short    wType                 : 4;      /* enum - see above */
   u_short    wReserved             : 2;      /* reserved */
   u_short    wInputVoltageProbeHandle;       /* handle of voltage probe (type 26) for this PS (0xFFFF == na) */
   u_short    wCoolingDeviceHandle;           /* handle of cooling device (type 27) for this PS (0xFFFF == na) */
   u_short    wInputCurrentProbeHandle;       /* handle of current probe (type 29) for this PS (0xFFFF == na) */
} SMBIOS_SYSTEM_POWER_SUPPLY, *PSMBIOS_SYSTEM_POWER_SUPPLY;

/* type 40 */
typedef struct _SMBIOS_ADDITIONAL_INFO
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byNumAddInfo;                   /* number of additional information entries */
   u_char    byInfo[1];                      /* raw additional information */
} SMBIOS_ADDITIONAL_INFO, *PSMBIOS_ADDITIONAL_INFO;

/* type 41 */
enum DeviceTypeEx_t
{
   dvtxOther            = 0x01,
   dvtxUnknown          = 0x02,
   dvtxVideo            = 0x03,
   dvtxSCSI             = 0x04,
   dvtxEthernet         = 0x05,
   dvtxTokenRing        = 0x06,
   dvtxSound            = 0x07,
   dvtxPATA             = 0x08,
   dvtxSATA             = 0x09,
   dvtxSAS              = 0x0A,
};
typedef struct _SMBIOS_ONBOARD_DEVICES_EX_INFORMATION
{
   SMBIOS_HEADER Header;                   /* common header */
   u_char    byReference;                    /* string - reference designation (silk screen label) */
   u_char    byDeviceType          : 7;
   u_char    byDevEnabled          : 1;
   u_char    byDevTypeInstance;              /* unique instance within a device type */
   u_short    wSegment;
   u_char    byBusNumber;
   u_char    byFunctionNumber      : 3;
   u_char    byDeviceNumber        : 5;
} SMBIOS_ONBOARD_DEVICES_EX_INFORMATION, *PSMBIOS_ONBOARD_DEVICES_EX_INFORMATION;

/* type 126 */
  /* SMBIOS_INSACTIVE - No additional data after the header */

/* type 127 */
  /* SMBIOS_END_OF_TABLE - No additional data after the header */


/*
 * SM BIOS Entry point structure
 */
typedef struct _SMBIOSEntryPoint
{
   u_char    sAnchorString[4];               /* anchor string (_SM_) */
   u_char    byChecksum;                     /* checksum of the entry point structure */
   u_char    byEPSLength;                    /* length of the entry point structure */
   u_char    byMajorVer;                     /* major version (02h for revision 2.1) */
   u_char    byMinorVer;                     /* minor version (01h for revision 2.1) */
   u_short    wMaxStructSize;                 /* size of the largest SMBIOS structure */
   u_char    byRevision;                     /* entry point structure revision implemented */
   u_char    byReserved[5];                  /* reserved */
   u_char    sIntermediateAS[5];             /* intermediate anchor string (_DMI_) */
   u_char    byIntermediateCS;               /* intermediate checksum */
   u_short    wSTLength;                      /* structure table length */
   u_int   dwSTAddr;                       /* structure table address */
   u_short    wNumStructures;                 /* number of structures present */
   u_char    byBCDRevision;                  /* BCD revision */
} SMBIOSEntryPoint;

/* type 195 */
typedef struct _CQSMBIOS_SERV_SYS_ID
{
   SMBIOS_HEADER Header;                   /* common header */
   unsigned char  serverSystemIdStr[32];
} CQSMBIOS_SERV_SYS_ID, *PCQSMBIOS_SERV_SYS_ID;

#pragma pack()

/* The following are defined in the OS specific files NTSMBIOS.C, NWSMBIOS.C, etc. */
//int ReadPhysMem(u_int dwStart, u_int dwSize, u_char * pBuf);

/* OS Independant routines. */
extern int InitSMBIOS();
extern void DeinitSMBIOS();
extern int ReinitSMBIOS();
extern int IsSMBIOSAvailable();
extern u_char SmbChecksum(u_char * pAddr, u_short wCount);
extern int SmbGetRecordByType(u_char byType, u_short wCopy, void **ppRecord);
extern int SmbGetRecordByHandle(u_short wHandle, void **ppRecord);
extern int SmbGetRecordByNumber(u_short wNumber, void **ppRecord);
extern u_char * SmbGetStringByNumber(void * pRecord, u_short wString);
extern int SmbGetRecord(u_char **ppRecord);

#endif  /*_SMBIOS_H */


