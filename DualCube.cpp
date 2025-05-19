#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>

const int mapping[6][4][3] = {
	{{0, 1, 0}, {1, 3, 0}, {3, 2, 0}, {2, 0, 0}},
	{{0, 4, 2}, {4, 5, 1}, {5, 1, 2}, {1, 0, 1}},
	{{0, 2, 1}, {2, 6, 2}, {6, 4, 1}, {4, 0, 2}},
	{{4, 6, 0}, {6, 7, 0}, {7, 5, 0}, {5, 4, 0}},
	{{2, 3, 1}, {3, 7, 2}, {7, 6, 1}, {6, 2, 2}},
	{{1, 5, 2}, {5, 7, 1}, {7, 3, 2}, {3, 1, 1}}};
const int FACT_OCT = 40320;

void codeToStatus(int code, int *status)
{
	int r1 = code % FACT_OCT;
	int r2 = code / FACT_OCT;
	int t = FACT_OCT;
	bool used[8] = {false};
	// memset(used, false, sizeof(bool) * 8);
	for (int i = 0; i < 8; i++)
	{
		t /= 8 - i;
		status[i] = r1 / t;
		status[15 - i] = r2 % 3;
		used[i] = false;
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
		used[status[i]] = true;
	}
}

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
	return r1 + FACT_OCT * r2;
}

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

void roll()
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
		clock_t start = clock();
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

		printf("%d\t%dms\n", next - end, clock() - start);
	}
	free(known);
	free(que);
}

int main(int argc, char *argv[])
{
	printf("Hello Cube\n");
	roll();
	return 0;
}
