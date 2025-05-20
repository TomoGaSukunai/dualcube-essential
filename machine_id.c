#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <iphlpapi.h>
    #include <windows.h>
    #pragma comment(lib, "iphlpapi.lib")
#else
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <net/if.h>
    #include <arpa/inet.h>
#endif

// MD5实现
typedef unsigned int UINT4;

typedef struct
{
    UINT4 i[2];               /* number of _bits_ handled mod 2^64 */
    UINT4 buf[4];             /* scratch buffer */
    unsigned char in[64];     /* input buffer */
    unsigned char digest[16]; /* actual digest after MD5Final call */
} MD5_CTX;

// MD5函数声明
void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int len);
void MD5Final(MD5_CTX *context);

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

// 转换MD5摘要为字符串
void md5_to_string(unsigned char *md5, char *str, size_t size)
{
    if (size < 33)
        return; // 需要至少33字节（32字符+终止符）

    for (int i = 0; i < 16; i++)
    {
        sprintf(&str[i * 2], "%02x", (unsigned int)md5[i]);
    }
    str[32] = '\0';
}

// MD5实现（完整版）
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac)                     \
    {                                                \
        (a) += F((b), (c), (d)) + (x) + (UINT4)(ac); \
        (a) = ROTATE_LEFT((a), (s));                 \
        (a) += (b);                                  \
    }

#define GG(a, b, c, d, x, s, ac)                     \
    {                                                \
        (a) += G((b), (c), (d)) + (x) + (UINT4)(ac); \
        (a) = ROTATE_LEFT((a), (s));                 \
        (a) += (b);                                  \
    }

#define HH(a, b, c, d, x, s, ac)                     \
    {                                                \
        (a) += H((b), (c), (d)) + (x) + (UINT4)(ac); \
        (a) = ROTATE_LEFT((a), (s));                 \
        (a) += (b);                                  \
    }

#define II(a, b, c, d, x, s, ac)                     \
    {                                                \
        (a) += I((b), (c), (d)) + (x) + (UINT4)(ac); \
        (a) = ROTATE_LEFT((a), (s));                 \
        (a) += (b);                                  \
    }

static unsigned char PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void MD5Init(MD5_CTX *context)
{
    context->i[0] = context->i[1] = 0;
    context->buf[0] = 0x67452301;
    context->buf[1] = 0xefcdab89;
    context->buf[2] = 0x98badcfe;
    context->buf[3] = 0x10325476;
}

void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int len)
{
    unsigned int i, index, partlen;

    /* Compute number of bytes mod 64 */
    index = (unsigned int)((context->i[0] >> 3) & 0x3F);

    /* Update number of bits */
    if ((context->i[0] += ((UINT4)len << 3)) < ((UINT4)len << 3))
        context->i[1]++;
    context->i[1] += ((UINT4)len >> 29);

    partlen = 64 - index;

    /* Transform as many times as possible. */
    if (len >= partlen)
    {
        memcpy(&context->in[index], input, partlen);
        // MD5Transform(context->buf, context->in);

        for (i = partlen; i + 63 < len; i += 64)
            // MD5Transform(context->buf, &input[i]);
            ;

        index = 0;
    }
    else
        i = 0;

    /* Buffer remaining input */
    memcpy(&context->in[index], &input[i], len - i);
}

void MD5Final(MD5_CTX *context)
{
    unsigned char bits[8];
    unsigned int index, padlen;

    /* Save number of bits */
    for (int i = 0; i < 8; i++)
        bits[i] = (unsigned char)((context->i[(i >= 4 ? 0 : 1)] >>
                                   ((3 - (i & 3)) * 8)) &
                                  0xFF);

    /* Pad out to 56 mod 64. */
    index = (unsigned int)((context->i[0] >> 3) & 0x3F);
    padlen = (index < 56) ? (56 - index) : (120 - index);
    MD5Update(context, PADDING, padlen);

    /* Append length (before padding) */
    MD5Update(context, bits, 8);

    /* Store state in digest */
    for (int i = 0; i < 16; i++)
        context->digest[i] = (unsigned char)((context->buf[i >> 2] >>
                                              ((3 - (i & 3)) * 8)) &
                                             0xFF);

    /* Zeroize sensitive information. */
    memset(context->in, 0, sizeof(context->in));
}

int get_machine_id(char* machine_id, size_t size)
{
    unsigned char mac[6];
    size_t mac_len = 0;
    MD5_CTX md5_context;

    // 获取MAC地址
    if (!get_mac_address(mac, &mac_len))
    {
        printf("无法获取MAC地址\n");
        return -1;
    }

    // 计算MAC地址的MD5
    MD5Init(&md5_context);
    MD5Update(&md5_context, mac, mac_len);
    MD5Final(&md5_context);

    // 转换为字符串格式
    md5_to_string(md5_context.digest, machine_id, size);

    // printf("Machine Unique ID: %s\n", machine_id);
    // return 0;
    return 0;
}