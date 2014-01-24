/*
 * sashba data access header
 */
#ifndef NETSNMP_ACCESS_SASHBA_CONFIG_H
#define NETSNMP_ACCESS_SASHBA_CONFIG_H

#define PCI_SSID_SPUTNIK_4INT     0x1704103C
#define PCI_SSID_SPUTNIK_8INT     0x3228103C
#define PCI_SSID_BLADE1           0x170D103C       // TAHITI
#define PCI_SSID_BLADE2           0x1312103C       // MERLION
#define PCI_SSID_BLADE3           0x132c103C       // BoraBora
#define PCI_SSID_VOSTOK           0x3229103C
#define PCI_SSID_GAGARIN          0x322B103C
#define PCI_SSID_ARES             0x322D103C
#define PCI_SSID_ROCKETS          0x3371103C
#define PCI_SSID_QUARTER          0x00441590
#define PCI_SSID_BELGIAN          0x00421590
#define PCI_SSID_COLT             0x00461590
#define PCI_SSID_MORGAN           0x00411590
#define PCI_SSID_ARABIAN          0x00431590
#define PCI_SSID_LSI_SAS_BASE     0x30001000
#define PCI_SSID_LSI_SAS_MASK     0xff00ffff

#define CPQ_REG_OTHER            1

// *****************************************
//     SAS HBA
// *****************************************

// SAS MIB host controller model values **
//      CPQ_REG_OTHER                  1 **
#define SAS_HOST_MODEL_SAS_HBA         2
#define SAS_HOST_MODEL_SPUTNIK         3
#define SAS_HOST_MODEL_CALLISTA        4
#define SAS_HOST_MODEL_VOSTOK          5
#define SAS_HOST_MODEL_GAGARIN         6
#define SAS_HOST_MODEL_ARES            7
#define SAS_HOST_MODEL_ROCKETS         8
#define SAS_HOST_MODEL_QUARTER         9
#define SAS_HOST_MODEL_BELGIAN         10
#define SAS_HOST_MODEL_COLT            11
#define SAS_HOST_MODEL_MORGAN          12
#define SAS_HOST_MODEL_ARABIAN         13

#define SPUTNIK_CNTL(model) ((model == SAS_HOST_MODEL_SPUTNIK)  ||   \
                             (model == SAS_HOST_MODEL_CALLISTA) ||   \
                             (model == SAS_HOST_MODEL_VOSTOK)   ||   \
                                (model == SAS_HOST_MODEL_GAGARIN)  ||   \
                             (model == SAS_HOST_MODEL_ARES)     ||   \
                             (model == SAS_HOST_MODEL_ROCKETS))

#define STOCKADE_CNTL(model) ((model == SAS_HOST_MODEL_QUARTER)  ||   \
                              (model == SAS_HOST_MODEL_BELGIAN)  ||   \
                              (model == SAS_HOST_MODEL_COLT)     ||   \
                              (model == SAS_HOST_MODEL_MORGAN)   ||   \
                              (model == SAS_HOST_MODEL_ARABIAN))


// SAS MIB host controller status values **
//      CPQ_REG_OTHER                      1 **
#define SAS_HOST_STATUS_OTHER          1
#define SAS_HOST_STATUS_OK             2
#define SAS_HOST_STATUS_FAILED         3

#define SAS_HOST_COND_OTHER          1
#define SAS_HOST_COND_OK             2
#define SAS_HOST_COND_DEGRADED       3
#define SAS_HOST_COND_FAILED         4

#endif /* NETSNMP_ACCESS_SASHBA_CONFIG_H */
