#ifndef _SYSINFO_H
#define _SYSINFO_H

typedef struct {
    char machine_id[33];
    char hostname[256];
    char os_name[256];
    char cpu_brand[49];
} sysInfo;

sysInfo getSysInfo(void);

// void get_cpuid_brand_string(char *brand);
// int get_hostname(char *buffer, size_t buffer_size);
// int get_os_name(char *buffer, size_t buffer_size);
// int get_machine_id(char *buffer, size_t bufferr_size);

#endif