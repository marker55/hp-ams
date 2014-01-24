/*
 * sasphydrv data access header
 */
#ifndef NETSNMP_ACCESS_SASPHYDRV_CONFIG_H
#define NETSNMP_ACCESS_SASPHYDRV_CONFIG_H

#define SAS_PHYS_STATUS_OTHER                          1
#define SAS_PHYS_STATUS_OK                             2
#define SAS_PHYS_STATUS_PREDICTIVEFAILURE              3
#define SAS_PHYS_STATUS_OFFLINE                        4
#define SAS_PHYS_STATUS_FAILED                         5
#define SAS_PHYS_STATUS_MISSINGWASOK                   6
#define SAS_PHYS_STATUS_MISSINGWASPREDICTIVEFAILURE    7
#define SAS_PHYS_STATUS_MISSINGWASOFFLINE              8
#define SAS_PHYS_STATUS_MISSINGWASFAILED               9
#define SAS_PHYS_STATUS_SSDWEAROUT                    10
#define SAS_PHYS_STATUS_MISSINGWASSSDWEAROUT          11
#define SAS_PHYS_STATUS_NOTAUTHENTICATED              12
#define SAS_PHYS_STATUS_MISSINGWASNOTAUTHENTICATED    13

#define SAS_PHYS_COND_OTHER     1
#define SAS_PHYS_COND_OK        2
#define SAS_PHYS_COND_DEGRADED  3
#define SAS_PHYS_COND_FAILED    4

/* Physical drive placement values
      CPQ_REG_OTHER                           1
*/
#define SAS_PHYS_DRV_PLACE_INTERNAL             2
#define SAS_PHYS_DRV_PLACE_EXTERNAL             3

/* Physical drive hot plug values **
      CPQ_REG_OTHER                           1
*/
#define SAS_PHYS_DRV_HOT_PLUG                   2
#define SAS_PHYS_DRV_NOT_HOT_PLUG               3

#define SAS_DISK_IS_ARRAY_MEMBER       2
#define SAS_DISK_IS_SPARE              3
#define SAS_DISK_NOT_ARRAY_MEMBER      4

#define SAS_PHYS_DRV_PLACE_INTERNAL             2
#define SAS_PHYS_DRV_PLACE_EXTERNAL             3

#define SAS_PHYS_DRV_ROTATING_PLATTERS          2
#define SAS_PHYS_DRV_SOLID_STATE                3

#define SAS_PHYS_DRV_ROT_SPEED_SSD              5

#endif /* NETSNMP_ACCESS_SASPHYDRV_CONFIG_H */
