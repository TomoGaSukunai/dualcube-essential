#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

#ifdef __WIN32__
#include <intrin.h>
#include <windows.h>
#endif

#ifdef __linux__
#include <linux/time.h>
// #include <unistd.h>
#endif
/**
 * 旋转映射矩阵
 * 见 DualCubeMath.ipynb
 */
const int mapping[6][4][3] = {
	{{0, 1, 0}, {1, 3, 0}, {3, 2, 0}, {2, 0, 0}},
	{{0, 4, 2}, {4, 5, 1}, {5, 1, 2}, {1, 0, 1}},
	{{0, 2, 1}, {2, 6, 2}, {6, 4, 1}, {4, 0, 2}},
	{{4, 6, 0}, {6, 7, 0}, {7, 5, 0}, {5, 4, 0}},
	{{2, 3, 1}, {3, 7, 2}, {7, 6, 1}, {6, 2, 2}},
	{{1, 5, 2}, {5, 7, 1}, {7, 3, 2}, {3, 1, 1}}};

const int FACTORIAL_OCT = 40320;			// 常数 8！
const int DUAL_CUBE_SPACE_LEGAL = 88179840; // 魔方旋转合法状态空间大小
const int DUAL_CUBE_SPACE_ALL = 264539520;	// 状态码状态空间大小

/**
 * 将状态码转换为状态填入给定的地址
 * @param code 状态码
 * @param status 状态
 */
void codeToStatus(int code, int *status)
{
	int r1 = code % FACTORIAL_OCT;
	int r2 = code / FACTORIAL_OCT;
	int t = FACTORIAL_OCT;
	int used[8] = {0};
	// memset(used, false, sizeof(bool) * 8);
	for (int i = 0; i < 8; i++)
	{
		t /= 8 - i;
		status[i] = r1 / t;
		status[15 - i] = r2 % 3;
		used[i] = 0;
		r1 %= t;
		r2 /= 3;
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j <= status[i]; j++)
		{
			if (used[j])
			{
				status[i]++;
			}
		}
		used[status[i]] = 1;
	}
}

/**
 * 计算状态对应的状态码
 * @param status 状态
 * @return 状态码
 */
int getCode(int *status)
{
	int r1 = 0;
	int r2 = 0;
	for (int i = 0; i < 8; i++)
	{
		int t = status[i];
		for (int j = 0; j < i; j++)
		{
			if (status[i] > status[j])
			{
				t--;
			}
		}
		r2 = r2 * 3 + status[8 + i];
		r1 = r1 * (8 - i) + t;
	}
	return r1 + FACTORIAL_OCT * r2;
}

/**
 * 旋转状态填入给定的地址
 * @param status 状态
 * @param way 旋转方式
 * @param ret 旋转后的状态
 */
void rotate(int *status, int way, int *ret)
{
	memcpy(ret, status, sizeof(int) * 16);
	const int (*map)[3] = mapping[way % 6];
	for (int i = 0; i < 4; i++)
	{
		const int *maplet = map[i];
		int src = way < 6 ? maplet[0] : maplet[1];
		int dst = way < 6 ? maplet[1] : maplet[0];
		int inc = way < 6 ? maplet[2] : (3 - maplet[2]) % 3;
		ret[dst] = status[src];
		ret[dst + 8] = (status[src + 8] + inc) % 3;
	}
}

/**
 * 满足pthread的参数要求,结构化进度条参数
 * @param running 运行状态
 * @param idx 当前索引
 * @param next 下一层索引
 */
typedef struct
{
	int *running;
	int *idx;
	int *next;
} bar_args;
bar_args bar_data;

/**
 * 进度条 *
 */
void *processBar(void *args)
{
	bar_args *arg = (bar_args *)args;
	char bar[11]; // 进度条
	bar[10] = '\0';
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};

	while (*(arg->running))
	{
		nanosleep(&ts, NULL);
		int idx_ = *(arg->idx) * 10 / DUAL_CUBE_SPACE_LEGAL;
		int next_ = *(arg->next) * 10 / DUAL_CUBE_SPACE_LEGAL;

		for (int i = 0; i < 10; i++)
		{
			if (idx_ > 0)
			{
				idx_--;
				next_--;
				bar[i] = '=';
			}
			else if (next_ > 0)
			{
				next_--;
				bar[i] = '+';
			}
			else
			{
				bar[i] = '-';
			}
		}
		fflush(stdout);
		printf("\r%s %d/%d/88179840\r", bar, *(arg->idx), *(arg->next));
		fflush(stdout);
	}
	return NULL;
}

int idx = 0;
int end = 1;
int next = 1;
int running = 1;

/**
 * 遍历所有状态
 *
 */
void traversal()
{

	short *known = (short *)malloc(sizeof(short) * DUAL_CUBE_SPACE_ALL);
	memset((void *)known, 0, sizeof(short) * DUAL_CUBE_SPACE_ALL);
	int *que = (int *)malloc(sizeof(int) * DUAL_CUBE_SPACE_LEGAL);
	memset(que, 0, sizeof(int) * DUAL_CUBE_SPACE_LEGAL);
	short s = 0x0fff;
	que[0] = 0;
	int status[16];
	int temp_s[16];

	char blank[81];
	memset(blank, ' ', 80);
	blank[80] = '\0';

	bar_data.running = &running;
	bar_data.idx = &idx;
	bar_data.next = &next;
	pthread_t thread;
	if (pthread_create(&thread, NULL, processBar, (void *)&bar_data) != 0)
	{
		perror("Failed to create thread");
		return;
	}

	while (idx < next)
	{
		end = next;
		struct timespec start;
		clock_gettime(CLOCK_REALTIME, &start);

		while (idx < end)
		{
			int nc = que[idx++];
			short k = known[nc];
			if (k == s)
			{
				continue;
			}
			codeToStatus(nc, status);
			for (int w = 0, mask = 1; w < 12; w++, mask <<= 1)
			{
				if ((k & mask) == 0)
				{
					short r_mask = 1 << ((w + 6) % 12);
					rotate(status, w, temp_s);
					int nn = getCode(temp_s);
					if (known[nn] == 0)
					{
						que[next++] = nn;
					}
					known[nn] |= r_mask;
				}
			}
			known[nc] = s;
		}

		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		long span = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
		printf("\r%s\r%d\t%lums%s\n", blank, next - end, span, blank);
	}
	running = 0;
	pthread_join(thread, NULL);
	printf("\n");	
	
	free(known);
	free(que);
}

int main(int argc, char *argv[])

{
	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);

	printf("Hello Cube\n");
	traversal();

	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	long span = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
	printf("total time: %lums\n", span);
	printf("Done\n");

	printf("Press enter to exit...\n");
	getchar();
	return 0;
}
