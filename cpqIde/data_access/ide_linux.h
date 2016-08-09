int get_unit_health(int);
char *get_unit_sn(int);

enum {
    CLASS_UNIQUE_ID,
    CLASS_PROC_NAME,
    CLASS_STATE,
    MAX_VALUE_FILES
};

static const char *sysfs_attr[MAX_VALUE_FILES] = {
    [CLASS_UNIQUE_ID] = "/unique_id",
    [CLASS_PROC_NAME] = "/proc_name",
    [CLASS_STATE] = "/state",

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

#define IDE_TRAP_DISK_STATUS_CHANGE          14004
#define IDE_TRAP_DISK_SSD_WEAR_STATUS_CHANGE 14006

// **************************************************************************
//  FUNCTION PROTOTYPES                                                    **
// **************************************************************************


