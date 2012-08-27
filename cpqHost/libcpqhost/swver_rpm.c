#include <rpm/header.h>
#include <fcntl.h>
#include <rpm/rpmmacro.h>
#include <rpm/rpmdb.h>
#include <strings.h>
#include <sys/stat.h>
#include <rpm/rpmlib.h>


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#define SWAP_BYTES(x)  ((unsigned short)(((x) & 0xff)<<8) | (((x)>>8) & 0xff))

#ifndef SNMP_MAXPATH
#define SNMP_MAXPATH 255
#endif
#define MAX_VERSION 50
#define MAX_DESCRIPTION 127

#define CPQHOSWVER_INDEX                1
#define CPQHOSWVER_STATUS               2
#define CPQHOSWVER_TYPE                 3
#define CPQHOSWVER_NAME                 4
#define CPQHOSWVER_DESCRIPTION          5
#define CPQHOSWVER_DATE                 6
#define CPQHOSWVER_LOCATION             7
#define CPQHOSWVER_VERSION              8
#define CPQHOSWVER_VERSIONBINARY        9

FindVarMethod   var_cpqHoSwVerTable;
struct variable4 cpqHoSwVer_variables[] = {
    {CPQHOSWVER_INDEX, ASN_INTEGER, RONLY,
         var_cpqHoSwVerTable, 2, {1, 1}},
    {CPQHOSWVER_STATUS, ASN_INTEGER, RONLY,
         var_cpqHoSwVerTable, 2, {1, 2}},
    {CPQHOSWVER_TYPE, ASN_INTEGER, RONLY,
         var_cpqHoSwVerTable, 2, {1, 3}},
    {CPQHOSWVER_NAME, ASN_OCTET_STR, RONLY,
         var_cpqHoSwVerTable, 2, {1, 4}},
    {CPQHOSWVER_DESCRIPTION, ASN_OCTET_STR, RONLY,
         var_cpqHoSwVerTable, 2, {1, 5}},
    {CPQHOSWVER_DATE, ASN_OCTET_STR, RONLY,
         var_cpqHoSwVerTable, 2, {1, 6}},
    {CPQHOSWVER_LOCATION, ASN_OCTET_STR, RONLY,
         var_cpqHoSwVerTable, 2, {1, 7}},
    {CPQHOSWVER_VERSION, ASN_OCTET_STR, RONLY,
         var_cpqHoSwVerTable, 2, {1, 8}},
    {CPQHOSWVER_VERSIONBINARY, ASN_OCTET_STR, RONLY,
         var_cpqHoSwVerTable, 2, {1, 9}},
};

oid     cpqHoSwVer_variables_oid[] = {  1, 3 , 6, 1, 4, 1, 232, 11, 2, 7, 2 };

typedef struct {
    char           *swi_directory;
    char            swi_name[SNMP_MAXPATH];     /* XXX longest file name */
    char            swi_version[MAX_VERSION];     /* XXX longest file name */
    int             swi_index;

    const char     *swi_dbpath;

    time_t          swi_timestamp;      /* modify time on database */
    int             swi_maxrec; /* no. of allocations */
    int             swi_nrec;   /* no. of valid offsets */
    int            *swi_recs;   /* db record offsets */
    rpmdb           swi_rpmdb;
    Header          swi_h;
    int             swi_prevx;
} SWI_t;

static SWI_t    _myswi = { NULL, "", "", 0 };       /* XXX static for now */


void
End_CH_SwVer(void)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */

    DEBUGMSGTL(("compaq/cpqHoSwVer", "End_CH_SwVer\n"));
    rpmdbClose(swi->swi_rpmdb); /* or only on finishing ? */
    swi->swi_rpmdb = NULL;
}


void init_cpqHoSwVer(void)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */
    struct stat     stat_buf;


    DEBUGMSGTL(("compaq/cpqHoSwVer", "init_cpqHoSwVer\n"));
    if (swi->swi_directory == NULL) {
        char            path[SNMP_MAXPATH];

        rpmReadConfigFiles(NULL, NULL);
        swi->swi_dbpath = rpmGetPath("%{_dbpath}", NULL);
        if (swi->swi_directory != NULL)
            free(swi->swi_directory);
        snprintf(path, sizeof(path), "%s/Packages", swi->swi_dbpath);
        if (stat(path, &stat_buf) == -1)
            snprintf(path, sizeof(path), "%s/packages.rpm", swi->swi_dbpath);
        path[ sizeof(path)-1 ] = 0;
        swi->swi_directory = strdup(path);
    }
    REGISTER_MIB("compaq/cpqHoSwVer", cpqHoSwVer_variables, variable4,
                cpqHoSwVer_variables_oid);
}

static void
Check_CHSV_cache(void *xxx)
{
    SWI_t          *swi = (SWI_t *) xxx;

    DEBUGMSGTL(("compaq/cpqHoSwVer", "Check_CHSV_cache\n"));
    /*
     * Make sure cache is up-to-date
     */
    if (swi->swi_recs != NULL) {
        struct stat     sb;

        lstat(swi->swi_directory, &sb);
        DEBUGMSGTL(("compaq/cpqHoSwVer", "swi->swi_timestamp = %ld, sb.st_mtime = %ld\n",swi->swi_timestamp,sb.st_mtime));

        if (swi->swi_timestamp == sb.st_mtime)
            return;
        swi->swi_timestamp = sb.st_mtime;
        swi->swi_maxrec = 0;
    }

    /*
     * Get header offsets
     */
    {
        int             ix = 0;
        int             offset;
        char *vendor;

        rpmdbMatchIterator mi = NULL;
        Header          h;
        mi = rpmdbInitIterator(swi->swi_rpmdb, RPMDBI_PACKAGES, NULL, 0);
        while ((h = rpmdbNextIterator(mi)) != NULL) {
            offset = rpmdbGetIteratorOffset(mi);

            if (ix >= swi->swi_maxrec) {
                swi->swi_maxrec += 256;
                swi->swi_recs = (swi->swi_recs == NULL)
                    ? (int *) malloc(swi->swi_maxrec * sizeof(int))
                    : (int *) realloc(swi->swi_recs,
                                      swi->swi_maxrec * sizeof(int));
            }
            headerGetEntry(h, RPMTAG_VENDOR, NULL, (void **) &vendor, NULL);
            if ((vendor != NULL) &&
                ((strncmp(vendor,"Hewlett",7) == 0) || (strcmp(vendor,"HP") == 0))) {
                swi->swi_recs[ix++] = offset;
            }
    
        }
        rpmdbFreeIterator(mi);

        swi->swi_nrec = ix;
    }
    End_CH_SwVer();
}

void
Init_cpqHo_SwVer(void)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */
    swi->swi_index = 0;

    DEBUGMSGTL(("compaq/cpqHoSwVer", "Init_cpqHo_SwVer:Entry\n "));
    if (swi->swi_rpmdb != NULL)
        return;
    DEBUGMSGTL(("compaq/cpqHoSwVer", "Init_cpqHo_SwVer:rpmdbOpen "));
    if (rpmdbOpen("", &swi->swi_rpmdb, O_RDONLY, 0644) != 0)
        swi->swi_index = -1;
    DEBUGMSGTL(("compaq/cpqHoSwVer", "Init_cpqHo_SwVer:Check_CHSV_cache "));
    Check_CHSV_cache(swi);
}


int
header_cpqHoSwVer(struct variable *vp,
                oid * name,
                size_t * length,
                int exact, size_t * var_len, WriteMethod ** write_method)
{
#define CPQHOSWVER_NAME_LENGTH    9
    oid             newname[MAX_OID_LEN];
    int             result;

    DEBUGMSGTL(("compaq/cpqHoSwVer", "header_cpqHoSwVer: "));
    DEBUGMSGOID(("compaq/cpqHoSwVer", name, *length));
    DEBUGMSG(("compaq/cpqHoSwVer", " %d\n", exact));

    memcpy((char *) newname, (char *) vp->name, vp->namelen * sizeof(oid));
    newname[CPQHOSWVER_NAME_LENGTH] = 0;
    result = snmp_oid_compare(name, *length, newname, vp->namelen + 1);
    if ((exact && (result != 0)) || (!exact && (result >= 0)))
        return (MATCH_FAILED);
    memcpy((char *) name, (char *) newname,
           (vp->namelen + 1) * sizeof(oid));
    *length = vp->namelen + 1;

    *write_method = (WriteMethod*)0;
    *var_len = sizeof(long);    /* default to 'long' results */
    return (MATCH_SUCCEEDED);
}

int
Get_Next_CH_SwVer(void)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */

    DEBUGMSGTL(("compaq/cpqHoSwVer", "Get_Next_CH_SwVer\n"));
    if (swi->swi_index == -1)
        return -1;

    /*
     * XXX Watchout: index starts with 1
     */
    if (0 <= swi->swi_index && swi->swi_index < swi->swi_nrec)
        return ++swi->swi_index;

    return -1;
}

void
Save_CH_SV_info(int ix)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */

    /*
     * XXX Watchout: ix starts with 1
     */
    DEBUGMSGTL(("compaq/cpqHoSwVer", "Save_CH_SV_info\n"));
    if (1 <= ix && ix <= swi->swi_nrec && ix != swi->swi_prevx) {
        int             offset;
        Header          h;
        char           *n, *v, *r;

        offset = swi->swi_recs[ix - 1];

        {
            rpmdbMatchIterator mi;
            mi = rpmdbInitIterator(swi->swi_rpmdb, RPMDBI_PACKAGES,
                                   &offset, sizeof(offset));
            if ((h = rpmdbNextIterator(mi)) != NULL)
                h = headerLink(h);
            rpmdbFreeIterator(mi);
        }

        if (h == NULL)
            return;
        if (swi->swi_h != NULL)
            headerFree(swi->swi_h);
        swi->swi_h = h;
        swi->swi_prevx = ix;

        headerGetEntry(swi->swi_h, RPMTAG_NAME, NULL, (void **) &n, NULL);
        headerGetEntry(swi->swi_h, RPMTAG_VERSION, NULL, (void **) &v,
                       NULL);
        headerGetEntry(swi->swi_h, RPMTAG_RELEASE, NULL, (void **) &r,
                       NULL);
        snprintf(swi->swi_name, sizeof(swi->swi_name), "%s", n);
        swi->swi_name[ sizeof(swi->swi_name)-1 ] = 0;

        snprintf(swi->swi_version, sizeof(swi->swi_version), "%s-%s", v, r);
        swi->swi_version[ sizeof(swi->swi_version)-1 ] = 0;
    }
}

void
Mark_CHSV_token(void)
{
}

void
Release_CHSV_token(void)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */
    DEBUGMSGTL(("compaq/cpqHoSwVer", "Release_CHSV_token\n"));

    if (swi != NULL && swi->swi_h) {
        headerFree(swi->swi_h);
        swi->swi_h = NULL;
        swi->swi_prevx = -1;
    }
}

int
header_cpqHoSwVerEntry(struct variable *vp,
                     oid * name,
                     size_t * length,
                     int exact,
                     size_t * var_len, WriteMethod ** write_method)
{
#define CPQHOSWVER_ENTRY_NAME_LENGTH      13
    oid             newname[MAX_OID_LEN];
    int             swinst_idx, LowIndex = -1;
    int             result;

    DEBUGMSGTL(("compaq/cpqHoSwVer", "var_cpqHoSwVerEntry: "));
    DEBUGMSGOID(("compaq/cpqHoSwVer", name, *length));
    DEBUGMSG(("compaq/cpqHoSwVer", " %d\n", exact));

    memcpy((char *) newname, (char *) vp->name, vp->namelen * sizeof(oid));
    /*
     * Find "next" installed software entry
     */

    Init_cpqHo_SwVer();
    while ((swinst_idx = Get_Next_CH_SwVer()) != -1) {
        DEBUGMSG(("compaq/cpqHoSwVer", "(index %d ....", swinst_idx));

        newname[CPQHOSWVER_ENTRY_NAME_LENGTH] = swinst_idx;
        DEBUGMSGOID(("compaq/cpqHoSwVer", newname, *length));
        DEBUGMSG(("compaq/cpqHoSwVer", "\n"));
        result = snmp_oid_compare(name, *length, newname, vp->namelen + 1);
        if (exact && (result == 0)) {
            LowIndex = swinst_idx;
            Save_CH_SV_info(LowIndex);
            break;
        }
        if ((!exact && (result < 0)) &&
            (LowIndex == -1 || swinst_idx < LowIndex)) {
            LowIndex = swinst_idx;
            Save_CH_SV_info(LowIndex);
        }
    }

    Mark_CHSV_token();
    End_CH_SwVer();

    if (LowIndex == -1) {
        DEBUGMSGTL(("compaq/cpqHoSwVer", "... index out of range\n"));
        return (MATCH_FAILED);
    }

    memcpy((char *) name, (char *) newname,
           (vp->namelen + 1) * sizeof(oid));
    *length = vp->namelen + 1;
    *write_method = (WriteMethod*)0;
    *var_len = sizeof(long);    /* default to 'long' results */

    DEBUGMSGTL(("compaq/cpqHoSwVer", "... get installed S/W stats "));
    DEBUGMSGOID(("compaq/cpqHoSwVer", name, *length));
    DEBUGMSG(("compaq/cpqHoSwVer", "\n"));
    return LowIndex;
}


unsigned char *var_cpqHoSwVerTable(struct variable * vp,
             oid * name,
             size_t * length,
             int exact, size_t * var_len, WriteMethod ** write_method)
{
    SWI_t          *swi = &_myswi;      /* XXX static for now */
    int             sw_idx = 0;
    static char     string[SNMP_MAXPATH];
    u_char         *ret = NULL;

    if (vp->magic < CPQHOSWVER_INDEX) {
        if (header_cpqHoSwVer(vp, name, length, exact, var_len, write_method)
            == MATCH_FAILED)
            return NULL;
    } else {

        sw_idx =
            header_cpqHoSwVerEntry(vp, name, length, exact, var_len,
                                 write_method);
        if (sw_idx == MATCH_FAILED)
            return NULL;
    }
    switch (vp->magic) {
    case CPQHOSWVER_INDEX:
        long_return = sw_idx;
        ret = (u_char *) & long_return;
        break;
    case CPQHOSWVER_STATUS:
        long_return = 2; /* loaded */
        ret = (u_char *) & long_return;
        break;
    case CPQHOSWVER_TYPE:
        {
            char *rpm_groups;
            if ( headerGetEntry(swi->swi_h, RPMTAG_GROUP, NULL, (void **) &rpm_groups, NULL) ) {
                if ( strstr(rpm_groups, "System Environment") != NULL )
                    long_return = 2;    /* operatingSystem */
                else
                    long_return = 5;    /* applcation */
            } else {
                long_return = 1;    /* unknown */
            }
            ret = (u_char *) & long_return;
        }
        break;
    case CPQHOSWVER_NAME:
        {
            strncpy(string, swi->swi_name, sizeof(string) - 1);

            /*
             * This will be unchanged from the initial "null"
             * value, if swi->swi_name is not defined
             */
            string[sizeof(string) - 1] = '\0';
            *var_len = strlen(string);
            ret = (u_char *) string;
        }
        break;

    case CPQHOSWVER_DESCRIPTION:
        {
            char         *rpm_description;
            if ( headerGetEntry(swi->swi_h, RPMTAG_DESCRIPTION, NULL, 
                                            (void **) &rpm_description, NULL) ) {
                strncpy(string,rpm_description, MAX_DESCRIPTION);
                string[sizeof(string) - 1] = '\0';
                *var_len = strlen(string);
            } else {
                string[0]=0;
                *var_len = 0;
            }
        }
        break;
    case CPQHOSWVER_DATE:
        {
            int32_t         *rpm_data;
            struct tm *t;

            if ( headerGetEntry(swi->swi_h, RPMTAG_INSTALLTIME, NULL, (void **) &rpm_data, NULL) ) {
                time_t          installTime = *rpm_data;
                t = localtime(&installTime);
                string[0] = (char)(SWAP_BYTES(t->tm_year + 1900));
                string[2] = (char)(t->tm_mon + 1);
                string[3] = (char)t->tm_mday;
                string[4] = (char)t->tm_hour;
                string[5] = (char)t->tm_min;
                string[6] = (char)t->tm_sec;
            } else {
                memset(&string[0],0,7);
            }
            *var_len = 7;
            return (unsigned char*)string;
        }
        break;
    case CPQHOSWVER_LOCATION:
        string[0] = '\0';
        *var_len = 0;
        ret = (u_char *) string;
        break;

    case CPQHOSWVER_VERSION:
    case CPQHOSWVER_VERSIONBINARY:
        strncpy(string, swi->swi_version, sizeof(string) - 1);
        string[sizeof(string) - 1] = '\0';
        *var_len = strlen(string);
        ret = (u_char *) string;
        break;

    default:
        DEBUGMSGTL(("compaq/cpqHoSwVer", "unknown sub-id %d in var_cpqHoSwVer\n",
                    vp->magic));
        ret = NULL;
        break;
    }
    Release_CHSV_token();
    return ret;


}

