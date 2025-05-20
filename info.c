#include <stdio.h>
#include <string.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#include <VersionHelpers.h>
#endif

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

