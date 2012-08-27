#ifndef SASLXLOW_H
#define SASLXLOW_H

char *get_unit_sn(int);
int get_unit_speed(int);
int get_unit_health(int);
int get_defect_data_size(int);

enum {
    DEVICE_SUBSYSTEM_DEVICE,
    DEVICE_SUBSYSTEM_VENDOR,
    CLASS_UNIQUE_ID,
    CLASS_VERSION_FW,
    CLASS_VERSION_BIOS,
    CLASS_MODEL,
    CLASS_STATE,
    DEVICE_STATE,
    DEVICE_REV,
    DEVICE_MODEL,
    DEVICE_VENDOR,
    BLOCK_SIZE,
    MAX_VALUE_FILES
};

static const char *sysfs_attr[MAX_VALUE_FILES] = {
    [DEVICE_SUBSYSTEM_DEVICE] = "/device/../subsystem_device",
    [DEVICE_SUBSYSTEM_VENDOR] = "/device/../subsystem_vendor",
    [CLASS_UNIQUE_ID] = "/unique_id",
    [CLASS_VERSION_FW] = "/version_fw",
    [CLASS_VERSION_BIOS] = "/version_bios",
    [CLASS_MODEL] = "/model",
    [CLASS_STATE] = "/state",
    [DEVICE_STATE] = "/device/state",
    [DEVICE_REV] = "/device/rev",
    [DEVICE_MODEL] = "/device/model",
    [DEVICE_VENDOR] = "/device/vendor",
    [BLOCK_SIZE] = "/size",

};

#define MAKE_CONDITION(total, new)  (new > total ? new : total)

/*
 *  DEVICE TYPES
 */

#define TYPE_DISK           0x00
#define TYPE_TAPE           0x01
#define TYPE_PROCESSOR      0x03    /* HP scanners use this */
#define TYPE_WORM           0x04    /* Treated as ROM by our system */
#define TYPE_ROM            0x05
#define TYPE_SCANNER        0x06
#define TYPE_MOD            0x07    /* Magneto-optical disk -
				     * - treated as TYPE_DISK */
#define TYPE_MEDIUM_CHANGER 0x08
#define TYPE_ENCLOSURE	    0x0d    /* Enclosure Services Device */
#define TYPE_NO_LUN         0x7f

#define ROT_SPEED_7200_MIN       7100
#define ROT_SPEED_7200_MAX       7300
#define ROT_SPEED_10K_MIN        9900
#define ROT_SPEED_10K_MAX        10100
#define ROT_SPEED_15K_MIN        14900
#define ROT_SPEED_15K_MAX        15100

#define SAS_TRAP_PHYSDRV_STATUS_CHANGE          5022

// **************************************************************************
//  FUNCTION PROTOTYPES                                                    **
// **************************************************************************


#endif // #ifndef SASLXLOW_H
