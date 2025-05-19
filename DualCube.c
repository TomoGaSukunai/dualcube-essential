#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>

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

/**
 * 常数 8！
 */
const int FACTORIAL_OCT = 40320;

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
 * 遍历所有状态
 *
 */
void traversal()
{

	short *known = (short *)malloc(sizeof(short) * 264539520);
	memset((void *)known, 0, sizeof(short) * 264539520);
	int *que = (int *)malloc(sizeof(int) * 88179840);
	memset(que, 0, sizeof(int) * 88179840);
	short s = 0x0fff;
	que[0] = 0;
	int idx = 0,
		end = 1,
		next = 1;
	int status[16];
	int temp_s[16];
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
		printf("%d\t%dms\n", next - end, span);
	}
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
	printf("total time: %dms\n", span);
	printf("Done\n");

	printf("Press enter to exit...\n");
	getchar();
	return 0;
}
