//
// (c) Copyright 2006-2015 Hewlett Packard Enterprise Development LP
// All rights reserved
//
// $ Id: $
//
//////////////////////////////////////////////////////////
/// \file nic_db.h
///
//////////////////////////////////////////////////////////


#ifndef NIC_DB_H
#define NIC_DB_H


//#include "common_structs.h"

/// number of entries (rows) in table
#define NIC_DB_ROWS 167

/// the hardware information table
static nic_hw_db nic_hw[NIC_DB_ROWS] = {
//{0x1077, 0x16A1, 0x1590, 0x00EC, "HP",  "764302-B21", "",           "",  "",  "",          43, "HPE FlexFabric 10Gb 4-port 536FLR-T Adapter", }, /* Gen9 Snap6 - Banjo */
//{0x14E4, 0x168E, 0x103C, 0x193A, "HP",  "700759-B21", "701534-001", "",  "",  "",          42, "HP FlexFabric 10Gb 2-port 533FLR-T Adapter", }, /* Gen9Snap5 - Bryan2 -same info as Bryan1 */
{0x10DF, 0x0720, 0x1590, 0x8153, "HP",  "N3U51A",     "",           "",  "",  "",          69, "HPE StoreFabric CN1200E 10GBASE-T Dual Port Converged Network Adapter", }, /* Gen9 Snap5 - Boxster5 */
{0x14E4, 0x168E, 0x1590, 0x8148, "HP",  "N3U52A",     "",           "",  "",  "",          33, "HPE StoreFabric CN1100R-T Adapter", }, /* Gen9 Snap5 - Barracuda-T */
{0x10DF, 0x0720, 0x103C, 0x8144, "HP",  "794525-B21", "",           "",  "",  "",          43, "HPE FlexFabric 10Gb 2-port 556FLR-T Adapter", }, /* Gen9 Snap5 - Earwig */
{0x14E4, 0x165F, 0x103C, 0x22E8, "HP",  "N/A",        "",           "",  "",  "",          40, "HP Ethernet 1Gb 2-port 332i Adapter", }, /* Gen9 Snap4 - embedded in Sakura,Yehliu */
{0x14E4, 0x16A2, 0x103C, 0x2231, "HP",  "777430-B21", "782833-001", "",  "",  "",          50, "HP Synergy 3820C 10/20Gb Converged Network Adapter", }, /* Bronco - Gen9 Snap4*/
{0x10DF, 0x0720, 0x103C, 0x2343, "",    "777434-B21", "782834-001", "",  "",  "",          51, "HP Synergy 3520C 10/20Gb Converged Network  Adapter", },  /* Ember - Gen9 Snap4 */
{0x8086, 0x1572, 0x103C, 0x22FD, "",    "727055-B21", "790316-001", "",  "",  "",          39, "HP Ethernet 10Gb 2-port 562SFP+ Adapter", }, /* Igloo - Gen9Snap3*/
{0x8086, 0x1572, 0x103C, 0x22FC, "",    "727054-B21", "790317-001", "",  "",  "",          43, "HP Ethernet 10Gb 2-port 562FLR-SFP+ Adapter", }, /* Indigo - Gen9Snap3 */
{0x8086, 0x1587, 0x103C, 0x22FE, "",    "727052-B21", "790318-001", "",  "",  "",          38, "HP Ethernet 20Gb 2-port 660FLB Adapter", }, /* Imp - Gen9Snap3 */
{0x8086, 0x1521, 0x103C, 0x8157, "",    "811546-B21", "816551-001", "",  "",  "",          35, "HP Ethernet 1Gb 4-port 366T Adapter", }, /* Izoro - Gen9Snap3 */
{0x15B3, 0x1007, 0x103C, 0x801F, "HP",  "779793-B21", "790314-001", "",  "",  "",          39, "HP Ethernet 10Gb 2-port 546SFP+ Adapter", }, /* Maze - Gen9Snap2  */
{0x15B3, 0x1007, 0x103C, 0x8020, "HP",  "779799-B21", "790315-001", "",  "",  "",          43, "HP Ethernet 10Gb 2-port 546FLR-SFP+ Adapter", }, /* Mesa - Gen9Snap2 */
{0x10DF, 0x0720, 0x103C, 0x803F, "",    "788995-B21", "792834-001", "",  "",  "",          39, "HP Ethernet 10Gb 2-port 557SFP+ Adapter", },  /* Elf - Gen9Snap2 */
{0x10DF, 0x0720, 0x103C, 0x220A, "",    "727060-B21", "764460-001", "",  "",  "",          45, "HP FlexFabric 10Gb 2-port 556FLR-SFP+ Adapter", }, /* Gen9Snap1 */
{0x10DF, 0x0720, 0x103C, 0x1935, "",    "700763-B21", "701536-001", "",  "",  "",          40, "HP FlexFabric 20Gb 2-port 650FLB Adapter", }, /* Gen9Snap1 */
{0x10DF, 0x0720, 0x103C, 0x1934, "",    "700767-B21", "701535-001", "",  "",  "",          38, "HP FlexFabric 20Gb 2-port 650M Adapter", }, /* Gen9Snap1 */
{0x14E4, 0x16A2, 0x103C, 0x22FA, "HP",  "766490-B21", "768080-001", "",  "",  "",          40, "HP FlexFabric 10Gb 2-port 536FLB Adapter", }, /* Gen9Snap1 */
{0x15B3, 0x1007, 0x103C, 0x22AC, "HP",  "N/A",        "",           "",  "",  "",          59, "HP XL230b InfiniBand FDR/Ethernet 40Gb 2-port 544+i Adapter", }, //Fury XL230b CX3 PRO LOM
{0x15B3, 0x1007, 0x103C, 0x800F, "HP",  "N/A",        "",           "",  "",  "",          59, "HP XL230b InfiniBand FDR/Ethernet 40Gb 2-port 544+i Adapter", }, //Fury XL230b CX3 PRO LOM
{0x10DF, 0x0720, 0x103C, 0x21D4, "",    "767078-001", "",           "",  "",  "",          64, "HP StoreFabric CN1200E 10Gb Converged Network Adapter", }, /* Boxster4 */
{0x15B3, 0x1007, 0x103C, 0x22F5, "",    "764286-B21", "764738-001", "",  "",  "",          51, "HP IB QDR/Ethernet 10Gb 2-port 544+FLR-QSFP Adapter",  }, /* Attitash M */
{0x15B3, 0x1007, 0x103C, 0x22F4, "",    "764285-B21", "764737-001", "",  "",  "",          56, "HP IB FDR/Ethernet 10Gb/40Gb 2-port 544+FLR-QSFP Adapter", },
{0x15B3, 0x1007, 0x103C, 0x22F3, "",    "764284-B21", "764736-001", "",  "",  "",          52, "HP IB FDR/Ethernet 10Gb/40Gb 2-port 544+QSFP Adapter", },
{0x15B3, 0x1007, 0x103C, 0x22F1, "HP",  "764282-B21", "764734-001", "",  "",  "",          48, "HP Infiniband QDR/Ethernet 10Gb 2P 544+M Adapter", }, //SugarLoaf
{0x15B3, 0x1007, 0x103C, 0x22F2, "HP",  "764283-B21", "764735-001", "",  "",  "",          53, "HP Infiniband FDR/Ethernet 10Gb/40Gb 2P 544+M Adapter", }, //SugarLoaf
{0x8086, 0x1521, 0x103C, 0x2003, "HP",  "N/A",        "",           "",  "",  "",          35, "HP Ethernet 1Gb 2-port 367i Adapter", }, /* Bantam */
{0x8086, 0x1521, 0x103C, 0x2159, "HP",  "N/A",        "",           "",  "",  "",          36, "HP Ethernet 10Gb 2-port 562i Adapter", }, /* Argos */
{0x8086, 0x1521, 0x103C, 0x2226, "HP",  "N/A",        "",           "",  "",  "",          28, "HP Ethernet 1Gb 1-port 364i Adapter", },        /* Wolf  */
{0x14E4, 0x16A2, 0x103C, 0x1916, "HP",  "700065-B21", "701527-001", "",  "",  "",          40, "HP FlexFabric 20Gb 2-port 630FLB Adapter", },  /* Gen8Snap6 */
{0x14E4, 0x16A2, 0x103C, 0x1917, "HP",  "700076-B21", "701528-001", "",  "",  "",          38, "HP FlexFabric 20Gb 2-port 630M Adapter", },  /* Gen8Snap6 */
{0x1077, 0x8020, 0x103C, 0x1957, "",    "629138-B21", "633962-001", "",  "",  "",          45, "HP FlexFabric 10Gb 2-port 526FLR-SFP+ Adapter", },  /* Gen8Snap5 */
{0x1077, 0x8020, 0x103C, 0x1958, "",    "655921-B21", "657130-001", "",  "",  "",          38, "HP FlexFabric 10Gb 2-port 526M Adapter", }, /* Quintana */
{0x1077, 0x8020, 0x103C, 0x1959, "",    "655918-B21", "657129-001", "",  "",  "",          40, "HP FlexFabric 10Gb 2-port 526FLB Adapter", }, /* Quitman */
{0x14E4, 0x168E, 0x103C, 0x1932, "HP",  "700741-B21", "701529-001", "",  "",  "",          40, "HP FlexFabric 10Gb 2-port 534FLB Adapter", },  /* Gen8Snap5 */
{0x14E4, 0x168E, 0x103C, 0x1931, "HP",  "701859-001", "",           "",  "",  "",          58, "HP StoreFabric CN1100R Dual Port Converged Network Adapter", },
{0x14E4, 0x168E, 0x103C, 0x1933, "HP",  "700748-B21", "701530-001", "",  "",  "",          38, "HP FlexFabric 10Gb 2-port 534M Adapter", },  /* Gen8Snap5 */
{0x14E4, 0x168E, 0x103C, 0x1930, "HP",  "700751-B21", "701531-001", "",  "",  "",          45, "HP FlexFabric 10Gb 2-port 534FLR-SFP+ Adapter", },  /* Gen8Snap5 */
{0x14E4, 0x168E, 0x103C, 0x193A, "HP",  "700759-B21", "701534-001", "",  "",  "",          43, "HPE FlexFabric 10Gb 2-port 533FLR-T Adapter", }, /* Gen8Snap5 - Bryan */
{0x8086, 0x1528, 0x103C, 0x192D, "HP",  "700699-B21", "701525-001", "",  "",  "",          40, "HP Ethernet 10Gb 2-port 561FLR-T Adapter", }, /* Gen8Snap5 */
{0x8086, 0x1528, 0x103C, 0x211A, "HP",  "716591-B21", "717708-001", "",  "",  "",          36, "HP Ethernet 10Gb 2-port 561T Adapter", },  /* Gen8Snap5 */
{0x8086, 0x10FB, 0x103C, 0x211B, "",    "716599-B21", "717709-001", "",  "",  "",          43, "HP Ethernet 10Gb 1-port 560FLR-SFP+ Adapter", },  /* Gen8Snap5  */
{0x14E4, 0x168E, 0x103C, 0x18D3, "HP",  "656596-B21", "657128-001", "",  "",  "",          36, "HP Ethernet 10Gb 2-port 530T Adapter", },  /* Gen8Snap4 */
{0x8086, 0x10FB, 0x103C, 0x17D0, "HP",  "665243-B21", "669281-001", "",  "",  "",          43, "HP Ethernet 10Gb 2-port 560FLR-SFP+ Adapter", },  /* Gen8Snap4 */
{0x8086, 0x10F8, 0x103C, 0x17D2, "HP",  "665246-B21", "669282-001", "",  "",  "",          36, "HP Ethernet 10Gb 2-port 560M Adapter", },  /* Gen8Snap4 */
{0x8086, 0x1521, 0x103C, 0x17D1, "HP",  "665240-B21", "669280-001", "",  "",  "",          33, "HP Ethernet 1Gb 4P 366FLR Adapter", },  /* Gen8Snap4 */
{0x8086, 0x10FB, 0x103C, 0x17D3, "HP",  "665249-B21", "669279-001", "",  "",  "",          39, "HP Ethernet 10Gb 2-port 560SFP+ Adapter", },  /* Gen8Snap3 */
{0x14E4, 0x168E, 0x103C, 0x339D, "HP",  "652503-B21", "656244-001", "",  "",  "",          39, "HP Ethernet 10Gb 2-port 530SFP+ Adapter", }, /* Gen8Snap2 */
{0x14E4, 0x165F, 0x103C, 0x1786, "HP",  "615732-B21", "616012-001", "",  "",  "",          35, "HP Ethernet 1Gb 2-port 332T Adapter", },  /* Gen8Snap2 */
{0x14E4, 0x165F, 0x103C, 0x2133, "HP",  "615732-B21", "",           "",  "",  "",          35, "HP Ethernet 1Gb 2-port 332i Adapter", },
{0x14E4, 0x1655, 0x103C, 0x18D2, "HP",  "N/A",        "",           "",  "",  "",          35, "HP Ethernet 1Gb 2-port 330i Adapter", },
{0x14E4, 0x165F, 0x103C, 0x22EB, "HP",  "N/A",        "",           "",  "",  "",          40, "HP Ethernet 1Gb 2-port 332i Adapter", },
{0x14E4, 0x1665, 0x103C, 0x22BD, "HP",  "N/A",        "",           "",  "",  "",          40, "HP Ethernet 1Gb 2-port 330i Adapter", },
{0x14E4, 0x1657, 0x103C, 0x22BE, "HP",  "N/A",        "",           "",  "",  "",          40, "HP Ethernet 1Gb 4-port 331i Adapter", },
{0x8086, 0x1521, 0x103C, 0x337F, "",    "N/A",        "",           "",  "",  "",          35, "HP Ethernet 1Gb 2-port 361i Adapter", },
{0x8086, 0x1521, 0x103C, 0x3380, "",    "N/A",        "",           "",  "",  "",          35, "HP Ethernet 1Gb 4-port 366i Adapter", },
{0x8086, 0x1523, 0x103C, 0x18D1, "",    "652500-B21", "656242-001", "",  "",  "",          37, "HP Ethernet 1Gb 2-port 361FLB Adapter", }, /* Gen8Snap2 */
{0x8086, 0x1523, 0x103C, 0x339F, "",    "615729-B21", "616010-001", "",  "",  "",          35, "HP Ethernet 1Gb 4-port 366M Adapter", }, /* Gen8Snap2 */
{0x8086, 0x1521, 0x103C, 0x339E, "",    "652497-B21", "656241-001", "",  "",  "",          35, "HP Ethernet 1Gb 2-port 361T Adapter", }, /* Gen8Snap2 */
{0x8086, 0x10F8, 0x103C, 0x18D0, "",    "655639-B21", "656243-001", "",  "",  "",          38, "HP Ethernet 10Gb 2-port 560FLB Adapter", }, /* Gen8Snap1 */
{0x14E4, 0x1657, 0x103C, 0x1904, "HP",  "N/A",        "",           "",  "",  "",          39, "HP Ethernet 1Gb 4-port 331i-SPI Adapter", },
{0x14E4, 0x1657, 0x103C, 0x3383, "HP",  "647594-B21", "649871-001", "",  "",  "",          35, "HP Ethernet 1Gb 4-port 331T Adapter", }, /* Gen8Snap1 */
{0x14E4, 0x1657, 0x103C, 0x3372, "HP",  "N/A",        "",           "",  "",  "",          35, "HP Ethernet 1Gb 4-port 331i Adapter", },
{0x14E4, 0x1657, 0x103C, 0x169D, "HP",  "629135-B21", "634025-001", "",  "",  "",          37, "HP Ethernet 1Gb 4-port 331FLR Adapter", }, /* Gen8Snap1 */
{0x14E4, 0x168E, 0x103C, 0x3382, "HP",  "647581-B21", "649869-001", "",  "",  "",          43, "HP Ethernet 10Gb 2-port 530FLR-SFP+ Adapter", }, /* Gen8Snap1 */
{0x14E4, 0x168E, 0x103C, 0x1798, "HP",  "656590-B21", "657132-001", "",  "",  "",          37, "HP Flex-10 10Gb 2-port 530FLB Adapter", }, /* Gen8Snap1 */
{0x14E4, 0x168E, 0x103C, 0x17A5, "HP",  "631884-B21", "657131-001", "",  "",  "",          35, "HP Flex-10 10Gb 2-port 530M Adapter", }, /* Gen8Snap1 */
{0x19A2, 0x0710, 0x103C, 0x337B, "",    "647586-B21",  "649940-001","",  "",    "",        40, "HP FlexFabric 10Gb 2-port 554FLB Adapter", }, /* Gen8Snap1 */
{0x19A2, 0x0710, 0x103C, 0x337C, "",    "647590-B21",  "649870-001","",  "",    "",        38, "HP FlexFabric 10Gb 2-port 554M Adapter", }, /* Gen8Snap1 */
{0x19A2, 0x0710, 0x103C, 0x3376, "",    "629142-B21",  "634026-001","",  "",    "",        45, "HP FlexFabric 10Gb 2-port 554FLR-SFP+ Adapter", }, /* Gen8Snap1 */
{0x19A2, 0x0710, 0x103C, 0x184E, "",    "674764-B21",  "675484-001","",  "",    "",        35, "HP Flex-10 10Gb 2-port 552M Adapter", }, /* Gen8Snap1 */
{0x8086, 0x10D3, 0x103C, 0x1785, "",    "N/A",        "",           "",  "",  "",          40, "HP NC112i 1-port Ethernet Server Adapter", },
{0x19A2, 0x0710, 0x103C, 0x3340, "",    "614203-B21",  "615406-001","",  "",    "",        47, "HP NC552SFP 2-port 10Gb Ethernet Server Adapter", },  /* G7 */
{0x19A2, 0x0710, 0x103C, 0x3341, "",    "610609-B21",  "610724-001","",  "",    "",        59, "HP NC552m 10Gb 2-port FlexFabric Converged Network  Adapter", },  /* G7 */
{0x19A2, 0x0710, 0x103C, 0x3345, "",    "613433-001",  "",          "",  "",    "",        58, "HP NC553m 10Gb 2-port FlexFabric Converged Network Adapter",  },
{0x1077, 0x8020, 0x103C, 0x336D, "",    "613436-001",  "",          "",  "",    "",        58, "HP NC525m 10Gb 2-port FlexFabric Converged Network Adapter", },
{0x1077, 0x8020, 0x103C, 0x3346, "",    "BS668-63002", "",          "",  "",    "",        59, "HP StorageWorks CN1000Q Dual Port Converged Network Adapter", },
{0x1077, 0x8020, 0x103C, 0x3379, "",    "N/A",         "",          "",  "",    "",        40, "HP NC376i 4-port Ethernet Server Adapter", },
{0x1077, 0x8020, 0x103C, 0x3378, "",    "N/A",         "",          "",  "",    "",        37, "HP NC376i 1G w/HP NC524SFP 10G Module", },
{0x1077, 0x8020, 0x103C, 0x3733, "",    "593717-B21",  "593742-001","",  "",    "",        55, "HP NC523SFP 10Gb 2-port Flex-10 Ethernet Server Adapter", },  /* G7 */
{0x14E4, 0x164C, 0x103C, 0x7037, "HP",  "012789-001", "395861-001", "X1","C", "x4 PCI-E",  51, "HP NC373T PCIe Multifunction Gigabit Server Adapter", },
{0x14E4, 0x16AC, 0x103C, 0x703D, "HP",  "012785-001", "395864-001", "X1","F", "x4 PCI-E",  51, "HP NC373F PCIe Multifunction Gigabit Server Adapter", },
{0x14E4, 0x164C, 0x103C, 0x7038, "HP",  "N/A",        "",           "",  "",  "x4 PCI-E",  46, "HP NC373i Multifunction Gigabit Server Adapter", },
{0x14E4, 0x164C, 0x103C, 0x7045, "HP",  "",           "",           "",  "",  "x4 PCI-E",  36, "HP NC374m PCIe Multifunction Adapter", },
{0x14E4, 0x16AC, 0x103C, 0x703B, "HP",  "N/A",        "",           "",  "",  "x4 PCI-E",  46, "HP NC373i Multifunction Gigabit Server Adapter", },
{0x14E4, 0x16AC, 0x103C, 0x1706, "HP",  "",           "",           "",  "",  "x4 PCI-E",  46, "HP NC373m Multifunction Gigabit Server Adapter", },
{0x14E4, 0x1639, 0x103C, 0x7055, "HP",  "N/A",        "",           "",  "",  "x4 PCI-E",  49, "HP NC382i DP Multifunction Gigabit Server Adapter", },
{0x14E4, 0x163A, 0x103C, 0x7056, "HP",  "N/A",        "",           "",  "",  "x4 PCI-E",  49, "HP NC382i DP Multifunction Gigabit Server Adapter", },
{0x14E4, 0x1639, 0x103C, 0x7059, "HP",  "453055-001", "458491-001", "X1","C", "x4 PCI-E",  54, "HP NC382T PCIe DP Multifunction Gigabit Server Adapter", },
{0x14E4, 0x163A, 0x103C, 0x171D, "HP",  "453244-001", "462748-001", "X1","S", "x4 PCI-E",  44, "HP NC382m DP 1GbE Multifunction BL-c Adapter", },
{0x14E4, 0x1668, 0x103C, 0x7039, "HP",  "N/A",        "",           "",  "",  "E-X Bridge",47, "HP NC324i PCIe Dual Port Gigabit Server Adapter", },
{0x14E4, 0x1669, 0x103C, 0x703A, "HP",  "N/A",        "",           "",  "",  "E-X Bridge",47, "HP NC324i PCIe Dual Port Gigabit Server Adapter", },
{0x14E4, 0x1678, 0x103C, 0x703E, "HP",  "N/A",        "",           "",  "",  "E-X Bridge",47, "HP NC326i PCIe Dual Port Gigabit Server Adapter", },
{0x14E4, 0x1679, 0x103C, 0x703C, "HP",  "N/A",        "",           "",  "",  "E-X Bridge",47, "HP NC326i PCIe Dual Port Gigabit Server Adapter", },
{0x14E4, 0x1679, 0x103C, 0x1707, "HP",  "406771-B21", "",           "",  "",  "E-X Bridge",32, "HP NC326m PCIe Dual Port Adapter", },
{0x14E4, 0x1679, 0x103C, 0x170C, "HP",  "416585-B21", "",           "",  "",  "E-X Bridge",47, "HP NC325m PCIe Quad Port Gigabit Server Adapter", },
{0x14E4, 0x1659, 0x103C, 0x7031, "HP",  "012429-001", "366603-001", "C", "C", "x1  PCI-E", 37, "HP NC320T PCIe Gigabit Server Adapter", },
{0x14E4, 0x1659, 0x103C, 0x7034, "HP",  "012543-001", "",           "X1","C", "x1  PCI-E", 57, "HP NC152T PCI Express 4-port Gigabit Combo Switch Adapter", },
{0x14E4, 0x1659, 0x103C, 0x7032, "HP",  "N/A",        "",           "",  "",  "x1  PCI-E", 37, "HP NC320i PCIe Gigabit Server Adapter", },
{0x14E4, 0x1659, 0x103C, 0x170B, "HP",  "",           "",           "",  "",  "x1  PCI-E", 22, "HP NC320m PCIe Adapter", },
{0x14E4, 0x165A, 0x103C, 0x7052, "OEM", "434917-001", "450079-001", "X2","C", "x1  PCI-E", 37, "HP NC105T PCIe Gigabit Server Adapter", },
{0x14E4, 0x165A, 0x103C, 0x7051, "HP",  "N/A",        "",           "",  "",  "x1  PCI-E", 37, "HP NC105i PCIe Gigabit Server Adapter", },
{0x14E4, 0x1648, 0x0E11, 0x00D1, "CPQ", "",           "",           "",  "",  "PCI-X",     32, "HP NC7783 Gigabit Server Adapter", },
{0x14E4, 0x0000, 0x0E11, 0x00E4, "CPQ", "",           "",           "",  "",  "PCI-X",     32, "HP NC7784 Gigabit Server Adapter", },
{0x14E4, 0x165B, 0x103C, 0x705D, "HP",  "N/A",        "",           "",  "",  "",          55, "HP NC107i integrated PCI Express Gigabit Server Adapter", },
{0x14E4, 0x166A, 0x103C, 0x7035, "HP",  "N/A",        "",           "",  "",  "E-X Bridge",47, "HP NC325i PCIe Dual Port Gigabit Server Adapter", },
{0x14E4, 0x166B, 0x103C, 0x7036, "HP",  "N/A",        "",           "",  "",  "E-X Bridge",47, "HP NC325i PCIe Dual Port Gigabit Server Adapter", },
{0x14E4, 0x164E, 0x103C, 0x171C, "OEM", "454521-001", "466308-001", "X1","BL","x8 PCI-E",  52, "HP NC532m Dual Port 10GbE Multifunction BL-c Adapter", },
{0x14E4, 0x1650, 0x103C, 0x171C, "OEM", "454521-001", "466308-001", "X1","BL","x8 PCI-E",  52, "HP NC532m Dual Port 10GbE Multifunction BL-c Adapter", },
{0x14E4, 0x164E, 0x103C, 0x7058, "HP",  "N/A",        "",           "",  "",  "X8  PCI-E", 52, "HP NC532i Dual Port 10GbE Multifunction BL-c Adapter", },
{0x14E4, 0x1650, 0x103C, 0x7058, "HP",  "N/A",        "",           "",  "",  "X8  PCI-E", 52, "HP NC532i Dual Port 10GbE Multifunction BL-c Adapter", },
{0x15B3, 0x5A44, 0x103C, 0x3107, "OEM", "374291-001", "374932-001", "A", "4x C","PCI-X",   43, "HP NC570C PCI-X Dual-port 4x Fabric Adapter", },
{0x15B3, 0x6278, 0x103C, 0x7033, "OEM", "374301-001", "374931-001", "B", "4x C","x4 PCI-E",49, "HP NC571C PCI Express Dual-port 4x Fabric Adapter", },
{0x15B3, 0x6764, 0x103C, 0x3313, "OEM", "539855-001", "539933-001", "X1","KR","x8 PCI-E",  46, "HP NC542m Dual Port Flex-10 10GbE BL-c Adapter", },
{0x15B3, 0x6764, 0x103C, 0x3310, "OEM", "",           "",           "",  "KR","x8 PCI-E",  38, "HP NC542m Dual Port 10GbE BL-c Adapter", },
{0x15B3, 0x6764, 0x103C, 0x3317, "HP",  "N/A",        "",           "",  "",  "x8 PCI-E",  46, "HP NC542i Dual Port Flex-10 10GbE BL-c Adapter", },
{0x15B3, 0x6746, 0x103C, 0x1781, "",    "N/A",        "",           "",  "",  "",          47, "HP NC543i 1-port 4x QDR IB/Flex-10 10Gb Adapter", },
{0x15B3, 0x6746, 0x103C, 0x3349, "",    "N/A",        "",           "",  "",  "",          53, "HP NC543i Integrated Flex-10  10GbE/4x QDR IB Adapter", },
{0x15B3, 0x1003, 0x103C, 0x18CE, "HP",  "661691-001", "",           "",  "",  "",          47, "HP Infiniband QDR/Ethernet 10Gb 2P 544M Adapter", },
{0x15B3, 0x1003, 0x103C, 0x18CF, "HP",  "661692-001", "",           "",  "",  "",          52, "HP Infiniband FDR/Ethernet 10Gb/40Gb 2P 544M Adapter", },
{0x15B3, 0x1003, 0x103C, 0x18D6, "HP",  "661685-001", "",           "",  "",  "",          55, "HP Infiniband FDR/Ethernet 10Gb/40Gb 2P 544QSFP Adapter", },
{0x15B3, 0x1003, 0x103C, 0x1777, "HP",  "661686-001", "",           "",  "",  "",          59, "HP Infiniband FDR/Ethernet 10Gb/40Gb 2P 544FLR-QSFP Adapter", },
{0x15B3, 0x1003, 0x103C, 0x18CD, "HP",  "661687-001", "",           "",  "",  "",          54, "HP Infiniband QDR/Ethernet 10Gb 2P 544FLR-QSFP Adapter", },
{0x15B3, 0x1003, 0x103C, 0x17C9, "HP",  "N/A",        "",           "",  "",  "",          47, "HP Infiniband QDR/Ethernet 10Gb 2P 544i Adapter", },
{0x4040, 0x0001, 0x103C, 0x7047, "OEM", "414124-001", "414158-001", "X1","SR","x8 PCI-E",  40, "HP NC510F PCIe 10 Gigabit Server Adapter", },
{0x4040, 0x0002, 0x103C, 0x7048, "OEM", "414127-001", "414159-001", "X1","CX4","x8 PCI-E", 40, "HP NC510C PCIe 10 Gigabit Server Adapter", },
{0x4040, 0x0100, 0x103C, 0x705B, "OEM", "468330-001", "468349-001", "X1","SFP","x8 PCI-E", 42, "HP NC522SFP Dual Port 10GbE Server Adapter", },
{0x4040, 0x0100, 0x103C, 0x1740, "OEM", "491176-001", "539931-001", "X1","CX4","x4 PCI-E", 54, "HP NC375T PCI Express Quad Port Gigabit Server Adapter", },
{0x4040, 0x0005, 0x103C, 0x170E, "OEM", "",           "",           "",  "",   "x8 PCI-E", 52, "HP NC512m Dual Port 10GbE Multifunction BL-c Adapter", },
{0x4040, 0x0100, 0x103C, 0x171B, "OEM", "454522-001", "466309-001", "X1","BL", "x8 PCI-E", 52, "HP NC522m Dual Port 10GbE Multifunction BL-c Adapter", },
{0x4040, 0x0100, 0x103C, 0x7057, "OEM", "N/A",         "",          "",  "",   "x8 PCI-E", 49, "HP NC522i Integrated Dual Port 10GbE BL-c Adapter", },
{0x4040, 0x0100, 0x103C, 0x705A, "HP",  "N/A",         "",          "",  "",   "x8 PCI-E", 56, "HP NC375i Quad Port Multifunction Gigabit Server Adapter", },
{0x4040, 0x0100, 0x103C, 0x3251, "HP",  "515787-001",  "",          "",  "",   "x8 PCI-E", 34, "HP NC375i 1G w/NC524SFP 10G Module", },
{0x19A2, 0x0700, 0x103C, 0x1746, "",    "581202-001",  "586445-001","X1","KR",  "x8 PcI-E",45, "HP NC550m DualPort Flex-10 10GbE BL-c Adapter", },
{0x19A2, 0x0700, 0x103C, 0x1747, "",    "581199-001",  "586444-001","X1","SFP+","x8 PCI-E",41, "HP NC550SFP DualPort 10GbE Server Adapter", },
{0x19A2, 0x0700, 0x103C, 0x174A, "",    "580153-001",  "",          "",  "KR",  "x8 PCI-E",61, "HP NC551m Dual Port FlexFabric 10Gb Converged Network Adapter", },
{0x19A2, 0x0700, 0x103C, 0x1749, "",    "585956-001",  "586446-001","X1","SFP+","x8 PCI-E",36, "HP NC550SFP Dual Port Server Adapter", },
{0x19A2, 0x0700, 0x103C, 0x174B, "",    "AW520A-kit",  "",          "",  "",    "",        59, "HP StorageWorks CN1000E Dual Port Converged Network Adapter", },
{0x19A2, 0x0700, 0x103C, 0x3314, "",    "N/A",         "",          "",  "",    "",        43, "HP NC551i Dual Port FlexFabric 10Gb Adapter", },
{0x19A2, 0x0710, 0x103C, 0x3315, "",    "N/A",         "",          "",  "",    "",        58, "HP NC553i 10Gb 2-port FlexFabric Converged Network Adapter", },
{0x19A2, 0x0700, 0x103C, 0x174A, "",    "580153-001",  "",          "",  "",    "",        43, "HP NC551m Dual Port FlexFabric 10Gb Adapter", },
{0x19A2, 0x0710, 0x103C, 0x3344, "",    "649108-001",  "",          "",  "",    "",        59, "HP StorageWorks CN1100E Dual Port Converged Network Adapter", },
{0x8086, 0x1027, 0x103C, 0x3103, "OEM", "367086-001", "367983-001", "A", "F", "PCI-X",     38, "HP NC310F PCI-X Gigabit Server Adapter", },
{0x8086, 0x10B5, 0x103C, 0x3109, "OEM", "389931-001", "389996-001", "A", "C", "PCI-X",     48, "HP NC340T PCI-X Quad-port Gigabit Server Adapter", },
{0x8086, 0x105E, 0x103C, 0x7044, "OEM", "412646-001", "412651-001", "A", "C", "x4  PCI-E", 40, "HP NC360T PCIe DP Gigabit Server Adapter", },
{0x8086, 0x10BC, 0x103C, 0x704B, "OEM", "435506-001", "436431-001", "A", "C", "x4  PCI-E", 47, "HP NC364T PCIe Quad Port Gigabit Server Adapter", },
{0x8086, 0x10D9, 0x103C, 0x1716, "OEM", "445976-001", "448068-001", "X1","S", "x4  PCI-E", 37, "HP NC360m Dual Port 1GbE BL-c Adapter", },
{0x8086, 0x10DA, 0x103C, 0x1717, "OEM", "447881-001", "448066-001", "X1","S", "x4  PCI-E", 37, "HP NC364m Quad Port 1GbE BL-c Adapter", },
{0x8086, 0x10B9, 0x103C, 0x704A, "OEM", "434903-001", "434982-001", "X2","C", "x1  PCI-E", 37, "HP NC110T PCIe Gigabit Server Adapter", },
{0x8086, 0x10D3, 0x103C, 0x3250, "OEM", "491175-001", "503827-001", "X1","C", "x1  PCI-E", 37, "HP NC112T PCIe Gigabit Server Adapter", },
{0x8086, 0x1081, 0x103C, 0x703F, "HP",  "N/A",        "",           "",  "",  "X4  PCI-E", 53, "HP NC360i Integrated Dual Port Gigabit Server Adapter", },
{0x8086, 0x10E7, 0x103C, 0x31FF, "",    "N/A",        "",           "",  "",  "",          51, "HP NC362i Integrated DP BL-c Gigabit Server Adapter", },
{0x8086, 0x10C9, 0x103C, 0x323F, "HP",  "N/A",        "",           "",  "C", "X4  PCI-E", 46, "HP NC362i Integrated DP Gigabit Server Adapter", },
{0x8086, 0x10C9, 0x103C, 0x31FF, "HP",  "N/A",        "",           "",  "S", "X4  PCI-E", 46, "HP NC362i Integrated DP Gigabit Server Adapter", },
{0x8086, 0x150E, 0x103C, 0x1780, "",    "593720-001", "",           "",  "",  "",          40, "HP NC365T 4-port Ethernet Server Adapter", },
{0x14E4, 0x16C7, 0x103C, 0x310E, "HP",  "",           "",           "",  "",  "PCI-X",     32, "HP NC7781 Gigabit Server Adapter", },
{0x14E4, 0x1648, 0x103C, 0x310F, "HP",  "",           "",           "",  "",  "PCI-X",     32, "HP NC7782 Gigabit Server Adapter", },
{0x14E4, 0x1654, 0x103C, 0x3100, "OEM", "353376-001", "353446-001", "A", "C", "PCI",       32, "HP NC1020 Gigabit Server Adapter", },
{0x14E4, 0x1654, 0x103C, 0x3100, "OEM", "395479-001", "395863-001", "A", "C", "PCI",       32, "HP NC1020 Gigabit Server Adapter", },
{0x14E4, 0x1654, 0x103C, 0x3226, "HP",  "012415-001", "366603-001", "A", "C", "PCI",       48, "NC150T PCI 4-port Gigabit Combo Switch & Adapter", },
{0x14E4, 0x164A, 0x103C, 0x3101, "HP",  "012361-001", "366606-001", "X1","C", "PCI-X",     46, "HP NC370T Multifunction Gigabit Server Adapter", },
{0x14E4, 0x16AA, 0x103C, 0x3102, "HP",  "012355-001", "366607-001", "A", "F", "PCI-X",     46, "HP NC370F Multifunction Gigabit Server Adapter", },
{0x14E4, 0x164A, 0x103C, 0x3070, "HP",  "012392-002", "374443-001", "A", "C", "x4 E-X B",  68, "HP NC380T PCI Express Dual Port Multifunction Gigabit Server Adapter", },
{0x14E4, 0x164A, 0x103C, 0x310A, "HP",  "",           "",           "",  "",  "PCI-X",     48, "HP ProLiant BL20p G3 Dual Port Multifunction NIC", },
{0x14E4, 0x164A, 0x103C, 0x310B, "HP",  "",           "",           "",  "",  "PCI-X",     51, "HP ProLiant BL25/35/45p Dual Port Multifunction NIC", },
{0x14E4, 0x164A, 0x103C, 0x3106, "HP",  "N/A",        "",           "",  "",  "PCI-X",     46, "HP NC370i Multifunction Gigabit Server Adapter", },
{0x14E4, 0x16AA, 0x103C, 0x310C, "HP",  "N/A",        "",           "",  "",  "PCI-X",     46, "HP NC370i Multifunction Gigabit Server Adapter", },
{0x14E4, 0x164A, 0x103C, 0x1709, "HP",  "N/A",        "",           "",  "",  "PCI-X",     46, "HP NC371i Multifunction Gigabit Server Adapter", }
};


#endif /* NIC_DB_H */

