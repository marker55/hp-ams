/* (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
   All rights reserved
 */
/*
 *  \file common_utilities.c
 *
 *  This file contains the code for the common utilities used by various
 *  projects.
 *
 */

#include "common_utilities.h"

/*
 *  \brief Trim string of chars from beginning and end of string
 *
 *  Trim chars in ptrim array from beginning and end of string adjusting the
 *  contents as needed - the location of the ptr is not changed. The chars
 *  can be interlaced within the beginning and end of the string - the 
 *  interlaced sequence of chars will be removed from the string.
 *  To remove spaces, tabs, and new-lines from a string pass " \n\t" in ptrim.
 *
 *  This is good for trimming such strings as (sting inclosed within []):
 *     pstring              ptrim     result
 *     [=" 10.10.10.10" ]                   [= "]        [10.10.10.10]
 *     ['10.10.10.10 ' ]                    [ ']         [10.10.10.10]
 *     [ NC 7170  ]                         [ ]          [NC 7170]
 *     ["=\" \n10.10.10.10\n \" \t\n\t\n]   [= \"\n\t]   [10.10.10.10]
 *
 *  unit tested by main
 *
 *  \author Tony Cureington
 *  \param[in,out] pstring ptr to string to trim
 *  \param[in] ptrim ptr to array of chars to trim from both ends of pstring
 *  \retval void
 *
 */
void trim_chars(char *pstring, char *ptrim)
{
    char *pstart;
    char *pend;
    int32_t i;
    int32_t num_trim_chars;
    int32_t trim_char_found;

    pstart = pstring;

    num_trim_chars = strlen(ptrim);

    /* trim from the beginning of the string */
    trim_char_found = 1;
    while (trim_char_found) {
        trim_char_found = 0;
        for (i=0; i<num_trim_chars; i++) {
            /* trim of one or more ptrim[i] chars */
            while ((ptrim[i] == *pstart)) {
                pstart++;
                trim_char_found = 1;
            }
        }
    }

    pend = &pstart[strlen(pstart)];
    if (pend == pstart) {
        /* no non-trim chars found */
        *pstring = '\0';
        return;
    }

    /* back pend off the null */
    pend--;

    /* trim from the end of the string. we know we have at least one
       non-trim char, so we don't need to worry about scanning off the
       beginning of the string. (yes we could merge the trimming
       of the beginning with the end, but it would complicate the code
       and is not worth it)
     */
    trim_char_found = 1;
    while (trim_char_found) {
        trim_char_found = 0;
        for (i=0; i<num_trim_chars; i++) {
            /* trim of one or more ptrim[i] chars */
            while ((ptrim[i] == *pend)) {
                *pend = '\0';
                pend--;
                trim_char_found = 1;
            }
        }
    }

    /* copy string to first location of string */
    memmove(pstring, pstart, strlen(pstart));

    pstring[strlen(pstart)] = '\0';

    return;
}

/*
 *  \brief Allocate the variables for the read_a_line function
 *
 *
 *  unit tested by detect_linux_distro
 *
 *  \author Tony Cureington
 *  \param[out] ppfile ptr-to-ptr to file handle var
 *  \param[out] ppa_line ptr-to-ptr of line var
 *  \param[out] pline_len line length var
 *  \retval void
 *
 *  \see read_a_line()
 *
 *  \note This function does not actually (yet) allocated any resources. Since
 *        a "free..." functions needs to be called it made since to name this
 *        function "alloc...".
 *
 */
void alloc_read_a_line_vars(FILE **ppfile,
        char **ppa_line,
        size_t *pline_len)
{
    *ppfile = (FILE*)0;
    *ppa_line = (char*)0;
    *pline_len = 0;
    return;
}


/*
 *  \brief free the variables for the read_a_line function
 *
 *  unit tested by detect_linux_distro
 *
 *  \author Tony Cureington
 *  \param[in,out] ppfile ptr-to-ptr to file handle var
 *  \param[in,out] ppa_line ptr-to-ptr of line var
 *  \param[in,out] pline_len line length var
 *  \retval void
 *
 *  \see read_a_line()
 *
 */
void free_read_a_line_vars(FILE **ppfile,
        char **ppa_line,
        size_t *pline_len)
{
    if ((FILE*)0 != *ppfile) {
        fclose(*ppfile);
        free(*ppa_line);

        *ppfile = (FILE*)0;
        *ppa_line = (char*)0;
        *pline_len = 0;
    }

    return;
}
/*
 *  \brief Read lines from a file
 *
 *  open and read a line from a file; if the file is not opened it is
 *  opened; the first line is read and returned; subsequent reads return
 *  sequential lines from the file;
 *
 *  unit tested by detect_linux_distro
 *
 *  \author Tony Cureington
 *  \param[in] pfile_name name of file to read lines from
 *  \param[in,out] ppfile file ptr
 *  \param[out] ppa_line line read from file
 *  \param[out] pline_len length of ppa_line
 *  \retval size_t number of bytes read success, -1 on error
 *
 *  \note the alloc_read_a_line_vars() must be called to init the vars
 *        passed to this function. done once per sequence of calls
 *        to the same file.
 *
 *  \note the free_read_a_line_vars() must be called to free the vars
 *        passed to this function. done once per sequence of calls
 *        to the same file.
 *
 *  \see alloc_read_a_line_vars() free_read_a_line_vars()
 *
 */
ssize_t read_a_line(char *pfile_name,
        FILE **ppfile,
        char **ppa_line,
        size_t *pline_len)
{
    if ((FILE*)0 == *ppfile) {
        /* first call to this function, so open the file */
        *ppfile = fopen(pfile_name, "r");
        if ((FILE *)0 == *ppfile) {
            DPRINTF("INFO: could not open %s (errno=%d)\n",
                    pfile_name, errno);
            return(-1);
        }
    }

    /* read and return a line from the file */
    return(getline(ppa_line, pline_len, *ppfile));
}

#ifdef DETECT_LINUX_DISTRO

/*
 *  \brief Detect linux distribution and version, and return official name
 *
 *  unit tested by main
 *
 *  \author Tony Cureington
 *  \param[out] pname linux distribution (RH, SUSE, VMWARE, etc)
 *  \param[out] ptype distro type (AS, ES, etc)
 *  \param[out] pversion distro version (SP1, U1, U2)
 *  \param[out] ppofficial_distro_name distro name from /etc/*-release file
 *              if ptr passed in is non-null, the calling app must free
 *              this string using free()
 *  \retval int32_t zero on successful linux distro detection, 
 *          non-zero otherwise
 *
 */
int32_t detect_linux_distro(linux_distro_name_t *pname,
        linux_distro_type_t *ptype,
        int32_t *pversion,
        char **ppofficial_distro_name)
{
    FILE *pfile;
    char *pline;
    char *pch;
    int32_t i;
    size_t line_len;

    char *(pdistro_file[]) =
    { "/etc/redhat-release",
        "/etc/SuSE-release",
        "/etc/SUSE-release",
    };

    int32_t pdistro_file_entries = sizeof(pdistro_file)/sizeof(char*);

    *pname = unknown_distro;
    *ptype = unknown_distro_type;
    *pversion = 0;

    alloc_read_a_line_vars(&pfile, &pline, &line_len);

    /* find the /etc/*-release file for the distro */
    for (i=0; i < pdistro_file_entries; i++) {
        if (access(pdistro_file[i],R_OK) >=0) {
            read_a_line(pdistro_file[i], &pfile, &pline, &line_len);
            /* we read a line from this file */
            break;
        }
    }

    /* save the original line from the file */
    if ((char**)0 != ppofficial_distro_name ) {
        *ppofficial_distro_name = STRDUP_NN(pline);
        trim_chars(*ppofficial_distro_name, " \t\n");
        DPRINTF("pofficial_distro_name = %s\n", *ppofficial_distro_name);
    }

    /* convert line to upper case for matching, above official name
       contains the original line. */
    for (pch=pline; '\0' != *pch; pch++) {
        *pch = toupper(*pch);
    }

    /*/ \todo add vwmare to list below */
    if (strstr(pline, "RED HAT")) {
        /* line is in the following format (but we converted it to upper case)

           Red Hat Enterprise Linux AS release 4 (Nahant Update 3)
         */

        *pname = rh_distro;

        /* get distro type */
        if (strstr(pline, "LINUX AS")) {
            *ptype = rh_as;
        } else if (strstr(pline, "LINUX ES")) {
            *ptype = rh_es;
        } else if (strstr(pline, "LINUX WS")) {
            *ptype = rh_ws;
        }

        /* get distro version */
        if ((char*)0 != (pch=strstr(pline, " RELEASE "))) {
            /* advance pch to the numeric version */
            pch += 9;
            *pversion = atoi(pch);
        } 
    } else if (strstr(pline, "SUSE")) {
        /* line is in the following format (but we converted it to upper case)

           SUSE Linux Enterprise Server 10 (x86_64)
           VERSION = 10
         */

        *pname = suse_distro;
        *ptype = suse_es;

        /* get distro version */
        if ((char*)0 != (pch=strstr(pline, " SERVER "))) {
            /* advance pch to the numeric version */
            pch += 8;
            *pversion = atoi(pch);
        } 
    } else if (strstr(pline, "FEDORA CORE")) {
        /* not a supported distro, just here for dev testing */
        *pname = fedora_core_distro;

        /* get distro version */
        if ((char*)0 != (pch=strstr(pline, " RELEASE "))) {
            /* advance pch to the numeric version */
            pch += 9;
            *pversion = atoi(pch);
        } 
    } else {
        DPRINTF("could not determine distro from: \"%s\"\n", pline);
        free_read_a_line_vars(&pfile, &pline, &line_len);
        return(1);
    }

    free_read_a_line_vars(&pfile, &pline, &line_len);

    DPRINTF("distro=%s, type=%s, version=%d\n",
            linux_distro_name[*pname],
            linux_distro_type[*ptype],
            *pversion);

    return(0);
}
#endif

/*
 *  \brief Unit test for this file
 *
 *  \author Tony Cureington
 *  \param[in] none
 *  \retval none
 *
 */
#ifdef UNIT_TEST

/* turn on/off unit tests */
#define UT_DECODE_INF_STRING                  0
#define UT_DETECT_LINUX_DISTRO                0
#define UT_TRIM_CHARS                         0
#define UT_GET_SLOT_INFO                      1
#define UT_GET_DYNAMIC_INFERFACE_INFO         0
#define UT_READ_TOKENS                        0
#define UT_GET_INTERFACE_LIST                 0

int main()
{

    printf("\n\nStarting Unit Test!\n\n");

    /*
     * UNIT_TEST detect_linux_distro
     */
#if DETECT_LINUX_DISTRO
    {
        linux_distro_name_t dist_name;
        linux_distro_type_t dist_type;
        int32_t dist_version;
        char *pname;

        /* call with NULL name string */
        if (0 == detect_linux_distro(&dist_name, &dist_type, 
                    &dist_version, (char **)0)) {
            printf("\ndistro = %s\ntype = %s\nversion = %d\n%s,%d\n\n",
                    linux_distro_name[dist_name],
                    linux_distro_type[dist_type],
                    dist_version, 
                    __FILE__, __LINE__);
        }

        /* call with non-NULL name string */
        if (0 == detect_linux_distro(&dist_name, &dist_type, 
                    &dist_version, &pname)) {
            printf("official distro name = %s\n%s,%d\n",
                    pname);
            free(pname);
        }
    }
#endif


    /*
     * UNIT_TEST trim_chars
     */
#if UT_TRIM_CHARS
#define TRIM_TESTS 4
    {
        typedef struct {
            char *pto_trim;
            char *ptrim_chars;
            char *ptrim_results;
        } trim_t;

        int i;
        char *pch;
        trim_t trim_test[TRIM_TESTS] =
        {
            {"=\" 10.10.10.10\" ",                "= \"",       "10.10.10.10"},
            {"'10.10.10.10 ' ",                   " '",         "10.10.10.10"},
            { " NC 7170  ",                       " ",          "NC 7170"},
            {"=\" \n10.10.10.10\n \" \t\n\t\n",   "= \"\n\t",   "10.10.10.10"}
        };

        for (i=0; i<TRIM_TESTS; i++) {

            pch = STRDUP_NN(trim_test[i].pto_trim);
            trim_chars(pch, trim_test[i].ptrim_chars);
            if (strcmp(pch, trim_test[i].ptrim_results) != 0) {
                printf("\n\nERROR: [%s] != [%s]\n%s,%d\n\n",
                        trim_test[i].pto_trim, trim_test[i].ptrim_results,
                        __FILE__, __LINE__);
                exit(1);
            }
            free(pch);
        }
    }
#endif


    printf("\n\nDONE!\n\n");
    return(0);
}
#endif // UNIT_TEST
