#ifndef FCA_DB_H
#define FCA_DB_H


//from snmp agents cmafcad/fcareg.h

/*****************************************/
/*     Fibre Channel Host Controller     */
/*****************************************/

/* FCA MIB host controller model values */
/*      FCA_OTHER                   1 */
#define FCA_HOST_MODEL_LP952        6
#define FCA_HOST_MODEL_LP8000       7
#define FCA_HOST_MODEL_QLA2340      8
#define FCA_HOST_MODEL_WHEATIES     9
#define FCA_HOST_MODEL_IPF_LP9802   10
#define FCA_HOST_MODEL_QLA23xx      11
#define FCA_HOST_MODEL_IPF_LP982    12
#define FCA_HOST_MODEL_2312DC       13
#define FCA_HOST_MODEL_IPF_QLA2312  14
#define FCA_HOST_MODEL_BL3xp        15
#define FCA_HOST_MODEL_BL2xp        16
#define FCA_HOST_MODEL_IPF_AB46xA   17
#define FCA_HOST_MODEL_FC_GENERIC   18
#define FCA_HOST_MODEL_QLA1143      19
#define FCA_HOST_MODEL_QLA1243      20
#define FCA_HOST_MODEL_EMU2143      21
#define FCA_HOST_MODEL_EMU2243      22
#define FCA_HOST_MODEL_EMU1050      23
#define FCA_HOST_MODEL_LPe1105      24
#define FCA_HOST_MODEL_QMH2462      25
#define FCA_HOST_MODEL_QLA1142SR    26
#define FCA_HOST_MODEL_QLA1242SR    27
#define FCA_HOST_MODEL_EMU2142SR    28
#define FCA_HOST_MODEL_EMU2242SR    29
#define FCA_HOST_MODEL_FCMCBL20PE   30
#define FCA_HOST_MODEL_QL81Q        31
#define FCA_HOST_MODEL_QL82Q        32
#define FCA_HOST_MODEL_QLQMH2562    33
#define FCA_HOST_MODEL_EMU81E       34
#define FCA_HOST_MODEL_EMU82E       35
#define FCA_HOST_MODEL_LPe1205      36
#define FCA_HOST_MODEL_SN1000ESP    37
#define FCA_HOST_MODEL_SN1000EDP    38
#define FCA_HOST_MODEL_SN1000QSP    39
#define FCA_HOST_MODEL_SN1000QDP    40
#define FCA_HOST_MODEL_SN1100ESP    41
#define FCA_HOST_MODEL_SN1100EDP    42

/* Shipping as of Gen8 Snap4 and Supported in gen8 */
#define FCA_HOST_MODEL_BROC8GBSP    43
#define FCA_HOST_MODEL_BROC8GBDP    44
#define FCA_HOST_MODEL_EMUCN1100E   45
#define FCA_HOST_MODEL_EMU554FLB    46
#define FCA_HOST_MODEL_EMU554M      47
#define FCA_HOST_MODEL_EMU554FLR    48
#define FCA_HOST_MODEL_EMULPE1205A  49
#define FCA_HOST_MODEL_QLCN1000Q    50
#define FCA_HOST_MODEL_QLQMH2572    51

/*  Gen8 Snap 5  */
#define FCA_HOST_MODEL_QL526FLR     52
#define FCA_HOST_MODEL_QLQMH2672    53
#define FCA_HOST_MODEL_BRCM534FLB   54
#define FCA_HOST_MODEL_BRCM534FLR   55
#define FCA_HOST_MODEL_BRCM534M     56
#define FCA_HOST_MODEL_BRCMCN1100R  57

/* Gen8 Snap 6  */
#define FCA_HOST_MODEL_EMULPE1605   58

/* Gen8 Snap 7  */
//#define FCA_HOST_MODEL_QL526M       59  - Cancelled 1/23/14
//#define FCA_HOST_MODEL_QL526FLB     60  - Cancelled 1/23/14
#define FCA_HOST_MODEL_BRCM630FLB   59
#define FCA_HOST_MODEL_BRCM630M     60

/* Gen9 Snap 1  */
#define FCA_HOST_MODEL_EMU556FLR    61
#define FCA_HOST_MODEL_EMU650M      62
#define FCA_HOST_MODEL_EMU650FLB    63
#define FCA_HOST_MODEL_EMUCN1200E   64
#define FCA_HOST_MODEL_BRCM536FLB   65    /* Breezy */

/* Gen9 Snap 4  -- Private Only -- */
/* Gen8 Snap 5 -- Public -- */
//#define FCA_HOST_MODEL_QNX531M      66    /* Bronco */
//#define FCA_HOST_MODEL_EMUEMBER     67    /* Ember */
//#define FCA_HOST_MODEL_EMUELECTRON  68    /* Electron */
//#define FCA_HOST_MODEL_QLAQUARTZ    69    /* Quartz */


/* Gen9 Snap 2 */
//#define FCA_HOST_MODEL_BRCM533FLR   63
//#define FCA_HOST_MODEL_BRCM536FLR   61 - Banjo - moved to Gen9 Snap2


/* HBA Board ID's */
#define ID_QLA2340         0x01000E11  /* QLogic QLA2340 id                */
#define ID_QLA2312DC       0x01010E11  /* QLogic QLA2312 dual channel id   */
#define ID_WHEATIES        0x01020E11  /* QLogic QLA2312 id                */
#define ID_BL3xp           0x01030E11  /* QLogic QLA2312 id                */
#define ID_BL2xp           0x01040E11  /* QLogic QLA2312 id                */
#define ID_QLA23xx         0x10000E11  /* QLogic QLA23?? id                */
#define ID_IPF_QLA2312     0x12BA103C  /* QLogic QLA2312 id for IPF        */
#define ID_LP952           0xF0950E11  /* Emulex LP952 id                  */
#define ID_IPF_LP982       0xF09810DF  /* Emulex LP982 id for IPF          */
#define ID_1050            0xF0A510DF  /* Emulex LP1050 family id (AB466A/AB467A - 1050E) */
#define ID_LP8000          0xF8000E11  /* Emulex LP8000 id                 */
#define ID_LP9002          0xF9000E11  /* Emulex LP9002 id                 */
#define ID_IPF_LP9802      0xF98010DF  /* Emulex LP9802 id for IPF         */
#define ID_QLA1143         0x12DD103C  /* QLogic QLA1143 id                */
#define ID_QLA1243         0x12D7103C  /* QLogic QLA1243 id                */
#define ID_LP1150          0xF0D510DF  /* Emulex LP1150 family id          */
#define ID_LP11000         0xFD0010DF  /* Emulex LP11000 family id         */
#define ID_LPe1105         0x1708103C  /* Emulex LPe1105 for blades id     */
#define ID_QMH2462         0x1705103C  /* QLogic QMH2462 for blades id     */
#define ID_QLA1142SR       0x7040103C  /* QLogic 1142SR id                 */
#define ID_QLA1242SR       0x7041103C  /* QLogic 1242SR id                 */
#define ID_EMUF0E5         0xF0E510DF  /* Emulex 0xF0E5 id                 */
#define ID_EMUFE00         0xFE0010DF  /* Emulex 0xFE00 id                 */
#define ID_FCMCBL20PE      0x1702103C  /* Emulex BL20p FC Mezz HBA         */
#define ID_QL81Q           0x3262103C  /* QLogic 81Q id                    */
#define ID_QL82Q           0x3263103C  /* QLogic 82Q dual port id          */
#define ID_QLQMH2562       0x3261103C  /* QLogic QMH2562 for blades id     */
#define ID_EMU81E          0x3281103C  /* Emulex 81E id                    */
#define ID_EMU82E          0x3282103C  /* Emulex 82E dual port id          */
#define ID_LPe1205         0x1719103C  /* Emulex LPe1205 for blades id     */
#define ID_SN1000ESP       0x17E4103C  /* Emulex SN1000E single port id    */
#define ID_SN1000EDP       0x17E5103C  /* Emulex SN1000E dual port id      */
#define ID_SN1000QSP       0x17E7103C  /* QLogic SN1000Q single port id    */
#define ID_SN1000QDP       0x17E8103C  /* QLogic SN1000Q dual port id      */

#define ID_BROC8GBSP       0x1743103C  /* Brocade 8GB single port id       */
#define ID_BROC8GBDP       0x1742103C  /* Brocade 8GB dual port id         */
#define ID_EMUCN1100E      0x3344103C  /* Emulex CN1100E dual port CNA id  */
#define ID_EMULPE1205A     0x338F103C  /* Emulex LPe1205A id               */
#define ID_QLCN1000Q       0x3348103C  /* QLogic CN1000Q CNA id            */
#define ID_EMU554FLR       0x3376103C  /* Emulex 554FLR flex fabric id     */
#define ID_EMU554M         0x337C103C  /* Emulex 554M flex fabric id       */
#define ID_EMU554FLB       0x337B103C  /* Emulex 554FLB flex fabric id     */
#define ID_QLQMH2572       0x338E103C  /* QLogic QMH2572 8GB id            */

#define ID_SN1100ESP       0x197E103C  /* Emulex SN1100E single port id    */
#define ID_SN1100EDP       0x197F103C  /* Emulex SN1100E dual port id      */
#define ID_QL526FLR        0x1957103C  /* QLogic 526FLR flex fabric id     */
#define ID_QLQMH2672       0x1939103C  /* QLogic QMH2672 8GB id            */
#define ID_BRCM534FLB      0x1932103C  /* Broadcom 534FLB flex fabric id   */
#define ID_BRCM534FLR      0x1930103C  /* Broadcom 534FLR flex fabric id   */
#define ID_BRCM534M        0x1933103C  /* Broadcom 534M flex fabric id     */
#define ID_BRCMCN1100R     0x1931103C  /* Broadcom CN1100R dual port id    */

#define ID_EMULPE1605      0x1956103C  /* Emulex LPe1605 id                */

/* Gen8 Snap7 */
//#define ID_QL526M          0x1958103C  /* QLogic 526M flex fabric id  - Cancelled 1/23/14     */
//#define ID_QL526FLB        0x1959103C  /* QLogic 526FLB flex fabric id  - Cancelled 1/23/14   */
#define ID_BRCM630FLB      0x1916103C  /* Broadcom 630FLB flex fabric id   */
#define ID_BRCM630M        0x1917103C  /* Broadcom 630M flex fabric id     */

/* Gen9 Snap 1  */
#define ID_EMU556FLR       0x220A103C  /* Emulex 556FLR-SFP+ (Escargot) */
#define ID_EMU650M         0x1934103C  /* Emulex 650M (Electra) */
#define ID_EMU650FLB       0x1935103C  /* Emulex 650FLB (Eureka) */
#define ID_EMUCN1200E      0x21D4103C  /* Emulex CN1200E (Boxster 4) */
#define ID_BRCM536FLB      0x22FA103C  /* Broadcom 536FLB (Breezy) */

/* Gen9 Snap 4 -- Private only --- */
/* Gen9 Snap5 -- Public --  */
//#define ID_QNX631M         0x2231103C  /* QLogic Bronco (TBird) CNA - Synergy 3820C  */
//#define ID_EMU_EMBER       0x2343103C  /* Emulex Ember (TBird) CNA - Synergy 3520C */
//#define ID_EMU_ELECTRON    0x8001103C  /* Emulex Electron (TBird) FCHBA - Synergy 3530C  */
//#define ID_QLA_QUARTZ2     0x8002103C  /* QLogic Quartz2 (TBird) FCHBA - Synergy 3830C  */

/* Gen9 Snap 2  */
//#define ID_BRCM533FLR      0x193A103C  /* Broadcom 533FLR flex fabric id (Bryan)  */
//#define ID_BRCM536FLR      0x22ED103C  /* Broadcom 536FLR-T (Banjo) */

typedef struct {
   unsigned long   ulBoardId;
   unsigned long   ulHbaApiModelLength;
   char            szHbaApiModel[32];
   unsigned char   bRegModel;
   char            szHbaHPModel[128];
} FCA_HBA_LIST, *PFCA_HBA_LIST;



FCA_HBA_LIST gFcaHbaList [] =
   {
      { ID_LP952,       0,    "",               FCA_HOST_MODEL_LP952,          "" },
      { ID_LP8000,      0,    "",               FCA_HOST_MODEL_LP8000,         "" },
      { ID_QLA2340,     0,    "",               FCA_HOST_MODEL_QLA2340,        "" },
      { ID_WHEATIES,    0,    "",               FCA_HOST_MODEL_WHEATIES,       "" },
      { ID_IPF_LP9802,  0,    "",               FCA_HOST_MODEL_IPF_LP9802,     "" },
      { ID_IPF_LP982,   0,    "",               FCA_HOST_MODEL_IPF_LP982,      "" },
      { ID_QLA23xx,     0,    "",               FCA_HOST_MODEL_QLA23xx,        "" },
      { ID_QLA2312DC,   0,    "",               FCA_HOST_MODEL_2312DC,         "" },
      { ID_IPF_QLA2312, 0,    "",               FCA_HOST_MODEL_IPF_QLA2312,    "" },
      { ID_BL3xp,       0,    "",               FCA_HOST_MODEL_BL3xp,          "" },
      { ID_BL2xp,       0,    "",               FCA_HOST_MODEL_BL2xp,          "" },
      { ID_1050,        0,    "",               FCA_HOST_MODEL_EMU1050,        "" },
      { ID_QLA1143,     0,    "",               FCA_HOST_MODEL_QLA1143,        "HP FC1143 4Gb PCI-X 2.0 HBA" },
      { ID_QLA1243,     0,    "",               FCA_HOST_MODEL_QLA1243,        "HP FC1243 4Gb PCI-X 2.0 FC HBA" },
      { ID_LP1150,      8,    "FC2143BR",       FCA_HOST_MODEL_EMU2143,        "HP FC2143 4Gb PCI-X 2.0 HBA" },
      { ID_LP11000,     8,    "FC2243BR",       FCA_HOST_MODEL_EMU2243,        "HP FC2243 4Gb PCI-X 2.0 FC HBA" },
      { ID_LPe1105,     0,    "",               FCA_HOST_MODEL_LPe1105,        "Emulex LPe1105-HP 4Gb FC HBA for HP c-Class Blade System" },
      { ID_QMH2462,     0,    "",               FCA_HOST_MODEL_QMH2462,        "Qlogic QMH2462 4Gb FC HBA for HP c-Class Blade System" },
      { ID_QLA1142SR,   0,    "",               FCA_HOST_MODEL_QLA1142SR,      "HP FC1142SR 4Gb PCI-e HBA" },
      { ID_QLA1242SR,   0,    "",               FCA_HOST_MODEL_QLA1242SR,      "HP FC1242SR 4Gb PCI-e FC HBA" },
      { ID_EMUF0E5,     6,    "A8002A",         FCA_HOST_MODEL_EMU2142SR,      "HP FC2142SR 4Gb PCI-e HBA" },
      { ID_EMUFE00,     6,    "A8003A",         FCA_HOST_MODEL_EMU2242SR,      "HP FC2242SR 4Gb PCI-e FC HBA" },
      { ID_FCMCBL20PE,  0,    "",               FCA_HOST_MODEL_FCMCBL20PE,     "Emulex based BL20p Fibre Channel Mezz HBA" },
      { ID_QL81Q,       0,    "",               FCA_HOST_MODEL_QL81Q,          "HP StorageWorks 81Q 8Gb PCI-e FC HBA" },
      { ID_QL82Q,       0,    "",               FCA_HOST_MODEL_QL82Q,          "HP StorageWorks 82Q 8Gb PCI-e Dual Port FC HBA" },
      { ID_QLQMH2562,   0,    "",               FCA_HOST_MODEL_QLQMH2562,      "QLogic QMH2562 8Gb FC HBA for HP BladeSystem c-Class" },
      { ID_EMU81E,      13,   "AJ762A/AH402A",  FCA_HOST_MODEL_EMU81E,         "HP StorageWorks 81E 8Gb PCI-e FC HBA" },
      { ID_EMU82E,      13,   "AJ763A/AH403A",  FCA_HOST_MODEL_EMU82E,         "HP StorageWorks 82E 8Gb PCI-e Dual Port FC HBA" },
      { ID_LPe1205,     10,   "LPe1205-HP",     FCA_HOST_MODEL_LPe1205,        "Emulex LPe 1205-HP 8Gb FC HBA for HP BladeSystem c-Class" },
      { ID_SN1000ESP,    0,   "",               FCA_HOST_MODEL_SN1000ESP,      "HP SN1000E 16Gb Single Port FC HBA" },
      { ID_SN1000EDP,    0,   "",               FCA_HOST_MODEL_SN1000EDP,      "HP SN1000E 16Gb Dual Port FC HBA" },
      { ID_SN1000QSP,    0,   "",               FCA_HOST_MODEL_SN1000QSP,      "HP SN1000Q 16Gb Single Port FC HBA" },
      { ID_SN1000QDP,    0,   "",               FCA_HOST_MODEL_SN1000QDP,      "HP SN1000Q 16Gb Dual Port FC HBA" },
      { ID_SN1100ESP,   0,    "",               FCA_HOST_MODEL_SN1100ESP,      "HP SN1100E 16Gb Single Port FC HBA"  },
      { ID_SN1100EDP,   0,    "",               FCA_HOST_MODEL_SN1100EDP,      "HP SN1100E 16Gb Dual Port FC HBA"  },
      { ID_BROC8GBSP,   6,    "AP769A",         FCA_HOST_MODEL_BROC8GBSP,      "HP StorageWorks 81B 8Gb Single Port PCI-e FC HBA" },
      { ID_BROC8GBDP,   6,    "AP770A",         FCA_HOST_MODEL_BROC8GBDP,      "HP StorageWorks 82B 8Gb Dual Port PCI-e FC HBA" },
      { ID_EMUCN1100E,  0,    "",               FCA_HOST_MODEL_EMUCN1100E,     "HP StorageWorks CN1100E Dual Port Converged Network Adapter" },
      { ID_EMU554FLB,   6,    "554FLB",         FCA_HOST_MODEL_EMU554FLB,      "HP FlexFabric 10Gb 2-port 554FLB Adapter"  },
      { ID_EMU554M,     4,    "554M",           FCA_HOST_MODEL_EMU554M,        "HP FlexFabric 10Gb 2-port 554M Adapter"  },
      { ID_EMU554FLR,   11,   "554FLR-SFP+",    FCA_HOST_MODEL_EMU554FLR,      "HP FlexFabric 10Gb 2-port 554FLR-SFP+ Adapter"  },
      { ID_EMULPE1205A, 0,    "",               FCA_HOST_MODEL_EMULPE1205A,    "HP Fibre Channel 8Gb LPe1205A Mezz"  },
      { ID_QLCN1000Q,   6,    "BS668A",         FCA_HOST_MODEL_QLCN1000Q,      "HP StorageWorks CN1000Q Dual Port Converged Network Adapter"  },
      { ID_QLQMH2572,   0,    "",               FCA_HOST_MODEL_QLQMH2572,      "HP QMH2572 8Gb FC HBA for c-Class BladeSystem"  },
      { ID_QL526FLR,    11,   "526FLR-SFP+",    FCA_HOST_MODEL_QL526FLR,       "HP FlexFabric 10Gb 2-port 526FLR-SFP+ Adapter"  },
      { ID_QLQMH2672,   0,    "",               FCA_HOST_MODEL_QLQMH2672,      "HP QMH2672 8Gb FC HBA for c-Class BladeSystem"  },
      { ID_BRCM534FLB,  6,    "534FLB",         FCA_HOST_MODEL_BRCM534FLB,     "HP FlexFabric 10Gb 2-port 534FLB Adapter"  },
      { ID_BRCM534FLR,  6,    "534FLR",         FCA_HOST_MODEL_BRCM534FLR,     "HP FlexFabric 10Gb 2-port 534FLR-SFP+ Adapter"  },
      { ID_BRCM534M,    4,    "534M",           FCA_HOST_MODEL_BRCM534M,       "HP FlexFabric 10Gb 2-port 534M Adapter"  },
      { ID_BRCMCN1100R, 0,    "",               FCA_HOST_MODEL_BRCMCN1100R,    "HP StoreFabric CN1100R Dual Port Converged Network Adapter"  },
      { ID_EMULPE1605,  0,    "",               FCA_HOST_MODEL_EMULPE1605,     "HP Fibre Channel 16Gb LPe1605 Mezz"  },
/* Gen8 Snap7 */
//      { ID_QL526M,      4,    "526M",           FCA_HOST_MODEL_QL526M,         "HP FlexFabric 10Gb 2-port 526M Adapter"  }, - Cancelled 1/23/14
//      { ID_QL526FLB,    6,    "526FLB",         FCA_HOST_MODEL_QL526FLB,       "HP FlexFabric 10Gb 2-port 526FLB Adapter"  }, - Cancelled 1/23/14
      { ID_BRCM630FLB,  6,    "630FLB",         FCA_HOST_MODEL_BRCM630FLB,     "HP FlexFabric 20Gb 2-port 630FLB Adapter"  },
      { ID_BRCM630M,    4,    "630M",           FCA_HOST_MODEL_BRCM630M,       "HP FlexFabric 20Gb 2-port 630M Adapter"  },
/* Gen9 Snap 1  */
      { ID_EMU556FLR,   11,   "556FLR-SFP+",    FCA_HOST_MODEL_EMU556FLR,      "HP FlexFabric 10Gb 2-port 556FLR-SFP+ Adapter"  },
      { ID_EMU650M,     11,   "650M",           FCA_HOST_MODEL_EMU650M,        "HP FlexFabric 20Gb 2-port 650M Adapter"  },
      { ID_EMU650FLB,   11,   "650FLB",         FCA_HOST_MODEL_EMU650FLB,      "HP FlexFabric 20Gb 2-port 650FLB Adapter"  },
      { ID_EMUCN1200E,  0,    "",               FCA_HOST_MODEL_EMUCN1200E,     "HP StoreFabric CN1200E 10Gb Converged Network Adapter" },
      { ID_BRCM536FLB,   6,   "536FLB",         FCA_HOST_MODEL_BRCM536FLB,     "HP FlexFabric 10Gb 2-port 536FLB Adapter"  },
/* Gen9 Snap 4 -- Private Only --  */
/* Gen9 Snap5 -- Public --  */
//      { ID_QNX631M,        5,    "3820C",       FCA_HOST_MODEL_QNX531M,        "HP Synergy 3820C 10/20Gb Converged Network Adapter "  },
//      { ID_EMU_EMBER,      5,    "3520C",       FCA_HOST_MODEL_EMUEMBER,       "HP Synergy 3520C 10/20Gb Converged Network  Adapter"  },
//      { ID_EMU_ELECTRON,   5,    "3530C",       FCA_HOST_MODEL_EMUELECTRON,    "HP Synergy 3530C 16G Fibre Channel Host Bus Adapter"  },
//      { ID_QLA_QUARTZ2,    5,    "3830C",       FCA_HOST_MODEL_QLAQUARTZ,      "HP Synergy 3830C 16G Fibre Channel Host Bus Adapter" },
/* Gen9 Snap 2  */
//      { ID_BRCM533FLR,  6,    "533FLR",         FCA_HOST_MODEL_BRCM533FLR,     "HP FlexFabric 10Gb 2-port 533FLR Adapter"  },
//      { ID_BRCM536FLR,   6,   "536FLR",         FCA_HOST_MODEL_BRCM536FLR,     "HP FlexFabric 10Gb 4-port 536FLR-T Adapter"  },
   };


#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))
#define MAX_FCA_HBA_LIST ARRAY_SIZE(gFcaHbaList)


#endif

