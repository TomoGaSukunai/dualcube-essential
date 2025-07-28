#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "src/DualCube.h"
#include "src/getSysInfo.h"
#include "src/post.h"

#define VERSION "0.0.2"

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为UTF-8
    printf("你好\n");

    // char machine_id[33]; // MD5字符串为32字符+终止符
    // if (get_machine_id(machine_id, sizeof(machine_id))) {
    //     printf("Failed to get machine ID\n");
    //     // return 1;
    //     machine_id[0] = '\0'; // 如果获取失败，设置为空字符串
    // }
    // printf("Machine ID: %s\n", machine_id);

    // char hostname[256] = {0};

    // if (get_hostname(hostname, sizeof(hostname)))
    // {
    //     printf("Failed to get hostname\n");
    //     // return 1;
    //     hostname[0] = '\0'; // 如果获取失败，设置为空字符串
    // }

    // printf("Name: %s\n", hostname);

    // char os_name[256] = {0};

    // if (get_os_name(os_name, sizeof(os_name)))
    // {
    //     printf("Failed to get OS name\n");
    //     // return 1;
    //     os_name[0] = '\0'; // 如果获取失败，设置为空字符串
    // }

    // printf("OS: %s\n", os_name);

    // char cpu_brand[49] = {0}; // 品牌字符串固定48字符

    // get_cpuid_brand_string(cpu_brand);

    // printf("CPU: %s\n", cpu_brand);

    sysInfo info = getSysInfo();


    printf("Machine ID: %s\n", info.machine_id);
    printf("Name: %s\n", info.hostname);
    printf("OS: %s\n", info.os_name);
    printf("CPU: %s\n", info.cpu_brand);

	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);

    
	printf("Hello Cube\n");
	traversal();

	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	long sec = now.tv_sec - start.tv_sec ;
    long nsec = now.tv_nsec - start.tv_nsec ;
	if (nsec < 0) 
    {
        sec--;
        nsec += 1000000000;
    }

	printf("total time: %lums\n", sec * 1000 + nsec / 1000000);
	printf("Done\n");  
    

#ifdef DEBUG
    return 0;
#else
    char **link = NULL;
    postmark(build_json(
        info.machine_id,
        info.hostname,
        info.os_name,
        info.cpu_brand,
        sec,
        nsec,
        VERSION
    ),link);


    printf("Press enter to exit...\n");
	getchar();
	return 0;
#endif
}
