#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <VersionHelpers.h>
#include <iphlpapi.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif

#include "getSysInfo.h"
#include "global.h"
#include "md5.h"

// 获取MAC地址

/**
 * @return 1 for success 
 */
int get_mac_address(unsigned char *mac, size_t *len)
{
#ifdef _WIN32
    IP_ADAPTER_ADDRESSES *adapter_addresses = NULL;
    ULONG out_buffer_size = 0;

    // 获取所需缓冲区大小
    DWORD result = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_addresses, &out_buffer_size);
    if (result != ERROR_BUFFER_OVERFLOW)
    {
        return 0;
    }

    // 分配内存
    adapter_addresses = (IP_ADAPTER_ADDRESSES *)malloc(out_buffer_size);
    if (!adapter_addresses)
        return 0;

    // 获取适配器地址
    result = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_addresses, &out_buffer_size);
    if (result != NO_ERROR)
    {
        free(adapter_addresses);
        return 0;
    }

    // 查找第一个非回环适配器
    IP_ADAPTER_ADDRESSES *adapter = adapter_addresses;
    while (adapter)
    {
        if (!(adapter->PhysicalAddressLength <= 0 || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK))
        {
            *len = (adapter->PhysicalAddressLength < 6) ? adapter->PhysicalAddressLength : 6;
            memcpy(mac, adapter->PhysicalAddress, *len);
            free(adapter_addresses);
            return 1;
        }
        adapter = adapter->Next;
    }

    free(adapter_addresses);
    return 0;
#else
    int sockfd;
    struct ifreq ifr;
    char ifname[IFNAMSIZ] = "eth0";

    // 尝试eth0接口
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return 0;
    }

    strcpy(ifr.ifr_name, ifname);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        // 尝试en0接口（适用于某些系统）
        strcpy(ifr.ifr_name, "en0");
        if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
        {
            close(sockfd);
            return 0;
        }
    }

    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    *len = 6;
    close(sockfd);
    return 1;
#endif
}

/**
 * 
 * @return 0 for success 
 */
int get_machine_id(char *machine_id, size_t len)
{
    if (len != 33)
    {
        return -1;
    }
    
    unsigned char mac[6];
    size_t mac_len;
    if (!get_mac_address(mac, &mac_len)){

        return -1;
    }
    MD5_CTX context;
    unsigned char digest[16];

    MD5Init(&context);
    MD5Update(&context, mac, 6);
    MD5Final(digest, &context);

    for (size_t i = 0; i < 16; i++)
    {
        snprintf(machine_id, 32, "%02x", digest[i]);
        machine_id += 2;
        // printf("%02x", digest[i]);
    }
    *machine_id = '\0';
    
    return 0;
}

void get_cpuid_brand_string(char *brand)
{
    uint32_t regs[4] = {0};
    char *ptr = brand;

    // 检查是否支持扩展品牌字符串功能
    __asm__ __volatile__(
        "cpuid"
        : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(0x80000000));

    if (regs[0] < 0x80000004)
    {
        strcpy(brand, "Brand string not supported");
        return;
    }

    // 获取三部分品牌字符串
    for (uint32_t i = 0x80000002; i <= 0x80000004; ++i)
    {
        __asm__ __volatile__(
            "cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(i));

        memcpy(ptr, &regs[0], sizeof(regs[0]));
        ptr += 4;
        memcpy(ptr, &regs[1], sizeof(regs[1]));
        ptr += 4;
        memcpy(ptr, &regs[2], sizeof(regs[2]));
        ptr += 4;
        memcpy(ptr, &regs[3], sizeof(regs[3]));
        ptr += 4;
    }

    *ptr = '\0';
}

int get_hostname(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size < 1)
        return -1;
    buffer[0] = '\0';

// Windows实现
#if defined(_WIN32)
    DWORD size = (DWORD)buffer_size;
    if (!GetComputerNameA(buffer, &size))
    {
        return -2; // 获取失败
    }

// POSIX系统实现 (Linux/macOS)
#else
    // 方法1：使用uname
    struct utsname name;
    if (uname(&name) == -1)
    {
        // 方法2：使用gethostname（回退方案）
        if (gethostname(buffer, buffer_size) != 0)
        {
            return -3;
        }
    }
    else
    {
        strncpy(buffer, name.nodename, buffer_size - 1);
    }
#endif

    buffer[buffer_size - 1] = '\0';
    return 0;
}

int get_os_name(char *buffer, size_t buffer_size)
{

    if (buffer == NULL || buffer_size < 1)
        return -1;
    buffer[0] = '\0';

// Windows实现
#if defined(_WIN32)
    HKEY hKey;
    DWORD dwSize = buffer_size;
    const char *subkey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return -2;
    }

    if (RegQueryValueExA(hKey, "ProductName", NULL, NULL, (LPBYTE)buffer, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return -3;
    }

    // 添加版本信息
    if (IsWindowsServer())
    {
        strcat_s(buffer, buffer_size, " Server");
    }
    else
    {
        strcat_s(buffer, buffer_size, " Client");
    }

    RegCloseKey(hKey);

// Linux实现（使用/etc/os-release）
#elif defined(__linux__)
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp)
    {
        char line[256];
        while (fgets(line, sizeof(line), fp))
        {
            if (strstr(line, "PRETTY_NAME"))
            {
                char *start = strchr(line, '=');
                if (start)
                {
                    start += 2; // 跳过等号和引号
                    char *end = strrchr(start, '"');
                    if (end)
                    {
                        *end = '\0';
                        strncpy(buffer, start, buffer_size - 1);
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    else
    { // 回退方案
        struct utsname uts;
        uname(&uts);
        snprintf(buffer, buffer_size, "%s %s", uts.sysname, uts.release);
    }

// macOS实现
#elif defined(__APPLE__)
    char version[128] = {0};
    size_t len = sizeof(version);

    // 获取macOS版本
    if (sysctlbyname("kern.osproductversion", version, &len, NULL, 0) == 0)
    {
        struct utsname uts;
        uname(&uts);
        snprintf(buffer, buffer_size, "macOS %s (Darwin %s)", version, uts.release);
    }
    else
    {
        strncpy(buffer, "macOS", buffer_size);
    }
#else
#error "Unsupported platform"
#endif

    return 0;
}

SysInfo GetSysInfo(){
    SysInfo info;
    get_machine_id(info.machine_id, sizeof(info.machine_id));
    get_hostname(info.hostname, sizeof(info.hostname));
    get_os_name(info.os_name, sizeof(info.os_name));
    get_cpuid_brand_string(info.cpu_brand);
    return info;
}