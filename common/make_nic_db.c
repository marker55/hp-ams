/* create nic_db.h from hw.csv
 * To Compile:
 *      cc -o make_nicdb make_nic_db.c  ../common/utils.c
 *
 * To run:
 *      ./make_nic_db > nic_db.h
 *
 */
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "utils.h"
#define HW_CSV "hw.csv"

typedef struct _nicdb {
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned short subsystem_vendor_id;
    unsigned short subsystem_device_id;
    char * cpq_oem;
    char * pca_pn;
    char * spares_pn;
    char * rev;
    char * number_of_ports;
    char * port_type;
    char * wol;
    char * pxe;
    char * rbsu;
    char * current;
    char * pcb_size_lh;
    char * description;
    char * bus_type;
    char * device_type;
    char * inf_name;
    char * hp_code_name;
    char * vendor_code_name;
    char * vendor_number;
    char * shipping_status;
    char * marketing_name;
} nicdb;

int main(int argc, char** argv) {

    int nic_db_rows = 0;
    nicdb **nic_db;
    int nic_db_size;

    read_line_t *nicdblines = NULL;

    if ((nicdblines = ReadFileLines(HW_CSV)) != NULL) {
        nic_db_size = sizeof(char *) * (nicdblines->count + 1);
        if ((nic_db = malloc(nic_db_size)) == NULL) 
            return;
        memset(nic_db,0, nic_db_size);
        int i = 0;
        nic_db_rows = nicdblines->count;

        printf("#ifndef NIC_DB_H\n");
        printf("#define NIC_DB_H\n");
    
        printf("#define NIC_DB_ROWS %d\n", nic_db_rows);
        printf("static nic_hw_db nic_hw[NIC_DB_ROWS] = {\n");

        i = nic_db_rows;
        while (i--)  {
            char *str = NULL;
            char *tokens[64];
            int tidx = 0;
            for (str = nicdblines->line[i]; ; tidx++) {
                tokens[tidx] = str;
                str = index(tokens[tidx],',');
                if (str == NULL)
                    break;
                *str = 0;
                str++;
                
            }
            if ((nic_db[i] = malloc(sizeof(nicdb))) != NULL) {
                int start = 15;
                int next = 16;
                sscanf(tokens[0], "%hxh", &nic_db[i]->vendor_id);
                sscanf(tokens[1], "%hxh", &nic_db[i]->device_id);
                sscanf(tokens[2], "%hxh", &nic_db[i]->subsystem_vendor_id);
                sscanf(tokens[3], "%hxh", &nic_db[i]->subsystem_device_id);
                nic_db[i]->cpq_oem = tokens[4];
                nic_db[i]->pca_pn = tokens[5];
                nic_db[i]->spares_pn = tokens[6];
                nic_db[i]->rev = tokens[7];
                nic_db[i]->number_of_ports = tokens[8];
                nic_db[i]->port_type = tokens[9];
                nic_db[i]->wol = tokens[10];
                nic_db[i]->pxe = tokens[11];
                nic_db[i]->rbsu = tokens[12];
                nic_db[i]->current = tokens[13];
                if ((strcmp(tokens[13],"\"3") == 0) && 
                    (strcmp(tokens[14], "3V\"") == 0)) {
                    tokens[13][2] = ' ';
                    next++;                
                    start++;
                    nic_db[i]->pcb_size_lh = tokens[15];
                } else 
                    nic_db[i]->pcb_size_lh = tokens[14];

                nic_db[i]->description = tokens[start];
                if ((tokens[start][0] != 0 ) && (tokens[start][0] == '"')) {
                    while (tokens[start][strlen(tokens[start]) - 1] != '"' ) {
                        tokens[start][strlen(tokens[start])] = ' ';
                        next++;
                    }
                }

                nic_db[i]->bus_type = tokens[next];
                nic_db[i]->device_type = tokens[next + 1];
                nic_db[i]->inf_name = tokens[next + 2];
                while (*nic_db[i]->inf_name == '"' ) 
                    nic_db[i]->inf_name++;
                while ((nic_db[i]->inf_name[strlen(nic_db[i]->inf_name) - 1] == ' ') ||
                       (nic_db[i]->inf_name[strlen(nic_db[i]->inf_name) - 1] == '"'))
                    nic_db[i]->inf_name[strlen(nic_db[i]->inf_name) - 1] = 0;
                nic_db[i]->hp_code_name = tokens[next + 3];
                nic_db[i]->vendor_code_name = tokens[next + 4];
                nic_db[i]->vendor_number = tokens[next + 5];
                nic_db[i]->shipping_status = tokens[next + 6];
                nic_db[i]->marketing_name = tokens[next + 7];
                printf("{0x%04X, 0x%04X, 0x%04X, 0x%04X,\n", 
                       nic_db[i]->vendor_id,
                       nic_db[i]->device_id,
                       nic_db[i]->subsystem_vendor_id,
                       nic_db[i]->subsystem_device_id);
                printf("            \"%s\", \"%s\", \"%s\", \"%s\",\n", 
                       nic_db[i]->cpq_oem, 
                       nic_db[i]->pca_pn,
                       nic_db[i]->spares_pn,
                       nic_db[i]->rev);
                printf("            \"%s\", \"%s\",\n", 
                       nic_db[i]->port_type, 
                       nic_db[i]->bus_type);
                printf("            %d,\n", strlen(nic_db[i]->inf_name));
                printf("            \"%s\",\n", nic_db[i]->inf_name);

                printf("            },\n");
            }
        }
        printf("};\n");
        printf("#endif /* NIC_DB_H */\n");

    }
}

