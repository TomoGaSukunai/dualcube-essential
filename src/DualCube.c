#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include "DualCube.h"

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef __linux__
#include <linux/time.h>
// #include <unistd.h>
#endif


#define KNOWN_T unsigned short
#define STATUS_T unsigned char
#define CODE_T unsigned int

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

int idx;
int end;
int next;
int running;

KNOWN_T s = 0x0fff;
/**
 * 将状态码转换为状态填入给定的地址
 * @param code 状态码
 * @param status 状态
 */
void codeToStatus(const CODE_T code, STATUS_T status[])
{
	CODE_T r1 = code % FACTORIAL_OCT;
	CODE_T r2 = code / FACTORIAL_OCT;
	CODE_T t = FACTORIAL_OCT;
	int used[8] = {0};
	// memset(used, false, sizeof(bool) * 8);
	for (int i = 0; i < 8; i++)
	{
		t /= 8 - i;
		status[i] = (STATUS_T)(r1 / t);
		status[15 - i] = (STATUS_T)(r2 % 3);
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
CODE_T getCode(const STATUS_T status[])
{
	CODE_T r1 = 0;
	CODE_T r2 = 0;
	for (int i = 0; i < 8; i++)
	{
		STATUS_T t = status[i];
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
void rotate(STATUS_T status[], int way, STATUS_T ret[])
{
	memcpy(ret, status, sizeof(STATUS_T) * 16);
	const int (*map)[3] = mapping[way % 6];
	for (int i = 0; i < 4; i++)
	{
		const int *maplet = map[i];
		const int src = way < 6 ? maplet[0] : maplet[1];
		const int dst = way < 6 ? maplet[1] : maplet[0];
		const int inc = way < 6 ? maplet[2] : (3 - maplet[2]) % 3;
		ret[dst] = status[src];
		ret[dst + 8] = (status[src + 8] + inc) % 3;
	}
}


/**
 * 进度条 *
 */
void *processBar(void *args){
	
	char bar[11]; // 进度条
	bar[10] = '\0';
	const struct timespec ts = {.tv_sec = 0, .tv_nsec = 20000000};

	while (running)
	{
		nanosleep(&ts, NULL);
		int idx_ = idx * 10 / DUAL_CUBE_SPACE_LEGAL;
		int next_ = next * 10 / DUAL_CUBE_SPACE_LEGAL;

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
		printf("\r%s %d/%d/88179840\r", bar, idx, next);
		fflush(stdout);
	}
	return NULL;
}


typedef struct  {
	void (* theCall)(traversalMsg);
}theCallStruct;
theCallStruct tcs;

void* processWithCall(void *args){
	theCallStruct * data = ((theCallStruct *)args);
	const struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
	while (running)
	{
		nanosleep(&ts, NULL);
		int a = idx;
		int b = DUAL_CUBE_SPACE_LEGAL;
		;
		data->theCall((traversalMsg){.msgType = TRAVERSAL_MSG_STEP,.msgData = {a,b}});
	}
	return NULL;
}


/**
 * 遍历所有状态
 *
 */
DLL_EXPORT void traversal()
{
	idx = 0;
	end = 1;
	next = 1;
	running = 1;

	KNOWN_T *known = (KNOWN_T *)malloc(sizeof(KNOWN_T) * DUAL_CUBE_SPACE_ALL);
	memset((void *)known, 0, sizeof(KNOWN_T) * DUAL_CUBE_SPACE_ALL);
	CODE_T *que = (CODE_T *)malloc(sizeof(CODE_T) * DUAL_CUBE_SPACE_LEGAL);
	memset(que, 0, sizeof(CODE_T) * DUAL_CUBE_SPACE_LEGAL);
	que[0] = 0;
	STATUS_T status[16];
	STATUS_T temp_s[16];

	char blank[81];
	memset(blank, ' ', 80);
	blank[80] = '\0';


	pthread_t thread;

	if (pthread_create(&thread, NULL, processBar, NULL) != 0)
	{
		// 创建线程失败
		perror("Failed to create thread");
		free(known);
		free(que);
		return;
	}


	while (idx < next)
	{
		end = next;
		struct timespec start;
		clock_gettime(CLOCK_REALTIME, &start);

		while (idx < end)
		{

			const CODE_T nc = que[idx++];
			const KNOWN_T k = known[nc];
			if (k == s)
			{
				continue;
			}
			codeToStatus(nc, status);
			for (int w = 0, mask = 1; w < 12; w++, mask <<= 1)
			{
				if ((k & mask) == 0)
				{
					KNOWN_T r_mask = 1 << ((w + 6) % 12);
					rotate(status, w, temp_s);
					CODE_T nn = getCode(temp_s);
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
		long long span = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
		printf("\r%s\r%d\t%lld ms%s\n", blank, next - end, span, blank);
	}
	running = 0;
	pthread_join(thread, NULL);
	free(known);
	free(que);
}

void traversalWithProgress( void (*callback)(traversalMsg))
	{
		idx = 0;
		end = 1;
		next = 1;
		running = 1;

		KNOWN_T *known = (KNOWN_T *)malloc(sizeof(KNOWN_T) * DUAL_CUBE_SPACE_ALL);
		memset((void *)known, 0, sizeof(KNOWN_T) * DUAL_CUBE_SPACE_ALL);
		CODE_T *que = (CODE_T *)malloc(sizeof(CODE_T) * DUAL_CUBE_SPACE_LEGAL);
		memset(que, 0, sizeof(CODE_T) * DUAL_CUBE_SPACE_LEGAL);
		que[0] = 0;
		STATUS_T status[16];
		STATUS_T temp_s[16];

		traversalMsg msg = {.msgType = TRAVERSAL_MSG_INFO};

		tcs.theCall = callback;
		pthread_t thread;
			if (pthread_create(&thread, NULL, processWithCall, &tcs) != 0) {
				// 创建线程失败
				perror("Failed to create thread");
				free(known);
				free(que);
				return;
			}

		while (idx < next)
		{
			end = next;
			struct timespec start;
			clock_gettime(CLOCK_REALTIME, &start);

			while (idx < end)
			{

				const CODE_T nc = que[idx++];
				const KNOWN_T k = known[nc];
				if (k == s)
				{
					continue;
				}
				codeToStatus(nc, status);
				for (int w = 0, mask = 1; w < 12; w++, mask <<= 1)
				{
					if ((k & mask) == 0)
					{
						KNOWN_T r_mask = 1 << ((w + 6) % 12);
						rotate(status, w, temp_s);
						CODE_T nn = getCode(temp_s);
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
			long long span = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
			snprintf(msg.msgData.str,254,"%d : %lld ms\n", next - end, span);
			callback(msg);
		}
		running = 0;
		pthread_join(thread, NULL);

		free(known);
		free(que);
}

