
#ifndef COMMON_PROTOTYPES_H
#define COMMON_PROTOTYPES_H

void trim_chars(char *pstring, char *ptrim);

void alloc_read_a_line_vars(FILE **ppfile, char **ppa_line, size_t *pline_len);

void free_read_a_line_vars(FILE **ppfile, char **ppa_line, size_t *pline_len);

ssize_t read_a_line(char *pfile_name, FILE **ppfile, 
                    char **ppa_line, size_t *pline_len);

void
free_ethtool_info_members(ethtool_info *pet_info);

int32_t
get_ethtool_info(char *pif_name,
                 ethtool_info *pet_info);

int32_t
get_pci_info(char *pbus_info,
             ushort *pci_ven,
             ushort *pci_dev,
             ushort *pci_sub_ven,
             ushort *pci_sub_dev);


nic_hw_db_t * get_nic_hw_info(ushort pci_ven,
                   ushort pci_dev,
                   ushort pci_sub_ven,
                   ushort pci_sub_dev);


int32_t
get_slot_info(char *pbus_info,
              int32_t *pslot,
              int32_t *pport,
              int32_t *pbay_slot,
              int32_t *pbay_port,
              int32_t *pvirtual_port,
              int32_t mlxport);

void
encode_string(char *pstring,
              int32_t len);

void
decode_string(char *pstring,
              int32_t len);

int32_t
get_slot_bay_info(int32_t bus, int32_t device, int32_t function,
                  int32_t* slot, int32_t* slot_port,
                  int32_t* bay, int32_t* bay_port, int32_t* virtual_port, int32_t mlxport);


link_status_t get_logical_interface_status(char *pif_name);

int32_t get_permanent_mac_addr(char *pif_name, char **pppermanent_mac);

int32_t
get_dynamic_interface_info(char* pif_name, dynamic_interface_info_t *pif);

void free_dynamic_interface_info(dynamic_interface_info_t *pif);

   
int32_t
get_interface_list(char ***pppif, char *pmatch_pattern, 
                   int32_t *pif_count);

int32_t
hpasm_installed(void);


#endif /* COMMON_PROTOTYPES_H */

