#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif
#include "./md5.c"

// 获取MAC地址
int get_mac_address(unsigned char *mac, size_t *len)
{
#ifdef _WIN32
    IP_ADAPTER_ADDRESSES *adapter_addresses = NULL;
    ULONG out_buffer_size = 0;
    DWORD result;

    // 获取所需缓冲区大小
    result = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_addresses, &out_buffer_size);
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
        if (adapter->PhysicalAddressLength > 0 &&
            !(adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK))
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


int get_machine_id(char *machine_id, size_t len)
{
    unsigned char mac[6];
    size_t mac_len;
    if (!get_mac_address(mac, &mac_len)){

        return 1;
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