#include "./DualCube.c"
#include "./info.c"
#include "./machine_id.c"
#include "./post.c"
#define VERSION "0.0.1"

int main(int argc, char *argv[])
{
    
    printf("\n");

    char machine_id[33]; // MD5字符串为32字符+终止符
    if (get_machine_id(machine_id, sizeof(machine_id))) {
        printf("Failed to get machine ID\n");
        // return 1;
        machine_id[0] = '\0'; // 如果获取失败，设置为空字符串
    }
    // printf("Machine ID: %s\n", machine_id);

    char hostname[256] = {0};

    if (get_hostname(hostname, sizeof(hostname)))
    {
        printf("Failed to get hostname\n");
        // return 1;
        hostname[0] = '\0'; // 如果获取失败，设置为空字符串
    }

    printf("Name: %s\n", hostname);

    char os_name[256] = {0};

    if (get_os_name(os_name, sizeof(os_name)))
    {
        printf("Failed to get OS name\n");
        // return 1;
        os_name[0] = '\0'; // 如果获取失败，设置为空字符串
    }

    printf("OS: %s\n", os_name);

    char cpu_brand[49] = {0}; // 品牌字符串固定48字符

    get_cpuid_brand_string(cpu_brand);

    printf("CPU: %s\n", cpu_brand);




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
    


    postmark(build_json(
        machine_id,
        hostname,
        os_name,
        cpu_brand,
        sec,
        nsec,
        VERSION
    ));
    

    printf("Press enter to exit...\n");
	getchar();
	return 0;
}
