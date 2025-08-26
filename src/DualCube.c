#include <stdint.h>
#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include "DualCube.h"

#ifdef __WIN32__
#include <intrin.h>
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
static const int mapping[6][4][3] = {
    {{4, 6, 0}, {6, 7, 0}, {7, 5, 0}, {5, 4, 0}},
    {{2, 3, 1}, {3, 7, 2}, {7, 6, 1}, {6, 2, 2}},
    {{1, 5, 2}, {5, 7, 1}, {7, 3, 2}, {3, 1, 1}},

    {{0, 1, 0}, {1, 3, 0}, {3, 2, 0}, {2, 0, 0}},
    {{0, 4, 2}, {4, 5, 1}, {5, 1, 2}, {1, 0, 1}},
    {{0, 2, 1}, {2, 6, 2}, {6, 4, 1}, {4, 0, 2}},
};

static const int DUAL_CUBE_FULL_SPACE = 264539520; // 状态码状态空间大小
static const int FACTORIAL_OCT = 40320;// 常数 8！

static int gR1Mod = 40320;
static int gSpaceLegal = 88179840; // 魔方旋转合法状态空间大小
static int gSpaceAll = 264539520;
static int gWAYS = 12;
static int gHALF = 6;
static KNOWN_T gKnownVisited = 0x0fff;
static int gBatchSize = 512;

static pthread_mutex_t mutexTraversal = PTHREAD_MUTEX_INITIALIZER; //lock for all

static pthread_mutex_t mutexWorker = PTHREAD_MUTEX_INITIALIZER; //lock for worker
static pthread_cond_t condWorker = PTHREAD_COND_INITIALIZER; //cond for worker

static pthread_mutex_t mutexMain = PTHREAD_MUTEX_INITIALIZER; //lock for main
static pthread_cond_t condMain = PTHREAD_COND_INITIALIZER; //cond for main

static BOOL gSleepMain = FALSE;
static const struct timespec ONE_MILLI = {.tv_sec = 0, .tv_nsec = 1000000L};


static CODE_T *gQueue;
static _Atomic KNOWN_T *gKnown;

static int gLevelStart;
static int gLevelEnd;

static _Atomic int gFetchIndex;
static _Atomic int gDoneCount;
static _Atomic int gNextLevelEnd;

static _Atomic int gWorkers;
static BOOL gRunning;


/**
 * 将状态码转换为状态填入给定的地址
 * @param code 状态码
 * @param status 状态
 */
static inline void codeToStatus(const CODE_T code, STATUS_T status[]) {
    CODE_T r1 = code % gR1Mod;
    CODE_T r2 = code / gR1Mod;
    CODE_T t = FACTORIAL_OCT;
    // int used[8] = {0};
    // memset(used, false, sizeof(bool) * 8);
    for (int i = 0; i < 8; i++) {
        t /= 8 - i;
        status[i] = (STATUS_T) (r1 / t);
        status[15 - i] = (STATUS_T) (r2 % 3);
        // used[i] = 0;
        r1 %= t;
        r2 /= 3;
    }
    uint32_t free_mask = 0x00ff;
    for (int i = 0; i < 8; i++) {
        uint32_t d = status[i];
        uint32_t pos_mask = _pdep_u32( 1 << d, free_mask);
        d = (STATUS_T)_tzcnt_u32(pos_mask);
        status[i] = (STATUS_T) d;
        free_mask &= ~pos_mask;
    }
    // for (int i = 0; i < 8; i++) {
    //     for (int j = 0; j <= status[i]; j++) {
    //         if (used[j]) {
    //             status[i]++;
    //         }
    //     }
    //     used[status[i]] = 1;
    // }
}

/**
 * 计算状态对应的状态码
 * @param status 状态
 * @return 状态码
 */
static inline CODE_T getCode(const STATUS_T status[]) {
    CODE_T r1 = 0;
    CODE_T r2 = 0;
    // for (int i = 0; i < 8; i++) {
    //     STATUS_T t = status[i];
    //     for (int j = 0; j < i; j++) {
    //         if (status[i] > status[j]) {
    //             t--;
    //         }
    //     }
    //     r2 = r2 * 3 + status[8 + i];
    //     r1 = r1 * (8 - i) + t;
    // }
    uint32_t mask = 0;

    // 使用位操作计算逆序数
    for (int i = 0; i < 8; i++) {
        uint8_t x = status[i];
        // 生成比x小的所有数字的掩码
        uint32_t smaller_mask = (1U << x) - 1;
        // 获取在已出现元素中比x小的元素
        uint32_t bits = mask & smaller_mask;
        // 计算个数
        uint32_t count = 0;
// #ifdef __POPCNT__
        count = _mm_popcnt_u32(bits); // 使用POPCNT指令
// #else
        // 通用popcount实现
        // bits =  (bits * 0x0202020202ULL & 0x010884422010ULL) & 0x0f;
// #endif

        r1 = r1 * (8 - i) + x - count;
        mask |= (1U << x); // 标记当前元素已使用
    }

    // 计算 r2（方向部分）
    for (int i = 0; i < 8; i++) {
        r2 = r2 * 3 + status[8 + i];
    }
    return r1 + gR1Mod * r2;
}

/**
 * 旋转状态填入给定的地址
 * @param status 状态
 * @param way 旋转方式
 * @param ret 旋转后的状态
 */
static inline void rotate(STATUS_T status[], int way, STATUS_T ret[]) {
    // memcpy(ret, status, sizeof(STATUS_T) * 16);
    __m128i* src_vec = (__m128i*)status;
    __m128i* dst_vec = (__m128i*)ret;
    dst_vec[0] = src_vec[0];
    const int (*map)[3] = mapping[way % gHALF];
    const int is_forward = (way < gHALF);
    for (int i = 0; i < 4; i++) {
        const int *maplet = map[i];
        const int src = is_forward ? maplet[0] : maplet[1];
        const int dst = is_forward ? maplet[1] : maplet[0];
        const int inc = is_forward ? maplet[2] : 3 - maplet[2];
        ret[dst] = status[src];
        ret[dst + 8] = (status[src + 8] + inc) % 3;
    }
}


typedef struct {
    void (*theCall)(traversalMsg);
} theCallStruct;

theCallStruct tcs;

void* processWithCall(void *args) {
    theCallStruct *data = ((theCallStruct *) args);
    if (data->theCall == NULL) {return NULL;}
    const struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
    while (gRunning) {
        nanosleep(&ts, NULL);
        int a = gDoneCount;
        int b = gSpaceLegal;
        int c = gNextLevelEnd;
        data->theCall((traversalMsg){.type = TRAVERSAL_MSG_STEP, .data = {a, b, c}});
    }
    return NULL;
}


/**
 *
 */
void *workerThread(void *args) {
    atomic_fetch_add(&gWorkers, 1);
    traversalMsg msg = {.type = TRAVERSAL_MSG_INFO};
    while (gRunning) {
        int _idx = atomic_fetch_add(&gFetchIndex, gBatchSize);
        if (_idx < gLevelEnd) {
            const int _aim = _idx + gBatchSize > gLevelEnd ? gLevelEnd : _idx + gBatchSize;
            _idx = _idx > gLevelStart ? _idx : gLevelStart;
            const int _works = _aim - _idx;
            while (_idx < _aim) {
                const CODE_T nc = gQueue[_idx++];
                KNOWN_T k = gKnown[nc];
                if (k != gKnownVisited) {
                    STATUS_T status[16];
                    STATUS_T temp_s[16];
                    codeToStatus(nc, status);
                    for (int w = 0, mask = 1; w < gWAYS; w++, mask <<= 1) {
                        if ((k & mask) == 0) {
                            const KNOWN_T r_mask = 1 << ((w + gHALF) % gWAYS);
                            rotate(status, w, temp_s);
                            CODE_T nn = getCode(temp_s);

                            KNOWN_T expected = 0;
                            while (!atomic_compare_exchange_weak(&gKnown[nn], &expected, r_mask)) {
                                if (expected != 0) {
                                    break;
                                }
                            }
                            if (expected == 0) {
                                int now = atomic_fetch_add(&gNextLevelEnd, 1);
                                gQueue[now] = nn;
                            }
                            gKnown[nn] |= r_mask;
                        }
                    }
                    gKnown[nc] = gKnownVisited;
                }
            }
            int _done = atomic_fetch_add(&gDoneCount, _works);
            if (_done + _works >= gLevelEnd) {
                pthread_mutex_lock(&mutexWorker);

                pthread_mutex_lock(&mutexMain);
                while (gSleepMain == FALSE) {
                    pthread_mutex_unlock(&mutexMain);
                    nanosleep(&ONE_MILLI, NULL);
                    nanosleep(&ONE_MILLI, NULL);
                    pthread_mutex_lock(&mutexMain);
                }
                pthread_mutex_unlock(&mutexMain);


                pthread_cond_signal(&condMain);

                atomic_fetch_add(&gWorkers, -1);
                pthread_cond_wait(&condWorker, &mutexWorker);
                atomic_fetch_add(&gWorkers, 1);

                pthread_mutex_unlock(&mutexWorker);
            }
        } else {
            pthread_mutex_lock(&mutexWorker);

            atomic_fetch_add(&gWorkers, -1);
            pthread_cond_wait(&condWorker, &mutexWorker);
            atomic_fetch_add(&gWorkers, 1);

            pthread_mutex_unlock(&mutexWorker);
        }
    }
    atomic_fetch_add(&gWorkers, -1);
    return NULL;
}

/**
 * 遍历所有状态
 *
 */
DLL_EXPORT int traversalDualCube(void (*callback)(traversalMsg), const unsigned short nThreads, const int mWays,
                                  const void **pQueue) {
    pthread_mutex_lock(&mutexTraversal);
    if (mWays == 6) {
        gSpaceAll = DUAL_CUBE_FULL_SPACE / 24;
        gSpaceLegal = DUAL_CUBE_FULL_SPACE / 72;
        gR1Mod = FACTORIAL_OCT /8;
        gBatchSize = 8;
    }else if (mWays == 12){
        gSpaceAll = DUAL_CUBE_FULL_SPACE;
        gSpaceLegal = DUAL_CUBE_FULL_SPACE / 3;
        gR1Mod = FACTORIAL_OCT;
        gBatchSize = 512;
    }else {
        return 1;
    }

    gWAYS = mWays;
    gHALF = gWAYS / 2;
    gKnownVisited = (1 << gWAYS) - 1;

    gLevelStart = 0;
    gLevelEnd = 0;

    atomic_store(&gNextLevelEnd, 0);
    atomic_store(&gFetchIndex, 0);
    atomic_store(&gDoneCount, 0);
    atomic_store(&gWorkers, 0);

    gRunning = TRUE;
    gSleepMain = FALSE;

    condWorker = PTHREAD_COND_INITIALIZER;
    mutexWorker = PTHREAD_MUTEX_INITIALIZER;
    condMain = PTHREAD_COND_INITIALIZER;
    mutexMain = PTHREAD_MUTEX_INITIALIZER;



    gKnown = (_Atomic KNOWN_T *) malloc(sizeof(_Atomic KNOWN_T) * gSpaceAll);
    memset(gKnown, 0, sizeof(_Atomic KNOWN_T) * gSpaceAll );
    gQueue = (CODE_T *) malloc(sizeof(CODE_T) * gSpaceLegal);
    // memset(gQueue, 0, sizeof(CODE_T) * gSpaceLegal);



    traversalMsg msg = {.type = TRAVERSAL_MSG_INFO};

    tcs.theCall = callback;


#ifdef __WIN32__
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    int CORES = sysinfo.dwNumberOfProcessors;
#else
    int CORES = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    int MAX_THREADS = nThreads > 0 ? nThreads : CORES;

    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, workerThread, NULL) != 0) {
            // 创建线程失败
            perror("Failed to create thread");
            free(gKnown);
            free(gQueue);
            return 2;
        }
    }

    pthread_t thread;

    if (pthread_create(&thread, NULL, processWithCall, &tcs) != 0) {
        // 创建线程失败
        perror("Failed to create thread");
        free(gKnown);
        free(gQueue);
        return 2;
    }
    while (atomic_load(&gWorkers) > 0) {
        nanosleep(&ONE_MILLI, NULL);
    }

    snprintf(msg.data.str,254,"Use %d threads\n", MAX_THREADS);
    callback(msg);

    gQueue[(int)atomic_fetch_add(&gNextLevelEnd, 1)] = 0;

    while (gLevelStart < atomic_load(&gNextLevelEnd)) {
        struct timespec start;
        clock_gettime(CLOCK_REALTIME, &start);

        while (atomic_load(&gWorkers) > 0) {
            nanosleep(&ONE_MILLI, NULL);
        }

        gLevelEnd = atomic_load(&gNextLevelEnd);

        atomic_store(&gFetchIndex, gLevelStart / gBatchSize * gBatchSize);

        pthread_mutex_lock(&mutexMain);

        pthread_cond_broadcast(&condWorker);
        gSleepMain = TRUE;
        pthread_cond_wait(&condMain, &mutexMain);
        gSleepMain = FALSE;
        // pthread_mutex_lock(&worker_mutex);
        // pthread_mutex_unlock(&worker_mutex);
        pthread_mutex_unlock(&mutexMain);

        if (callback != NULL) {
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            long long span = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;

            snprintf(msg.data.str, 254, "%d : %lld ms\n", atomic_load(&gNextLevelEnd) - gLevelEnd, span);
            callback(msg);
        }
        while (atomic_load(&gWorkers) > 0) {
            nanosleep(&ONE_MILLI, NULL);
        }
        gLevelStart = gLevelEnd;
    }
    gRunning = FALSE;


    while (atomic_load(&gWorkers) > 0) {
        nanosleep(&ONE_MILLI, NULL);
    }

    pthread_cond_broadcast(&condWorker);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_cond_broadcast(&condWorker);
        pthread_join(threads[i], NULL);
    }

    pthread_join(thread, NULL);

    // clear lock
    pthread_mutex_destroy(&mutexWorker);
    pthread_cond_destroy(&condWorker);
    pthread_mutex_destroy(&mutexMain);
    pthread_cond_destroy(&condMain);

    free(gKnown);
    if (pQueue == NULL) {
        free(gQueue);
    } else {
        *pQueue = gQueue;
        gQueue = NULL;
    }
    pthread_mutex_unlock(&mutexTraversal);
    return 0;
}

DLL_EXPORT void traversalWithProgressAndParas(void (*callback)(traversalMsg), const unsigned short nThreads, const int mWays) {
    traversalDualCube(callback, nThreads, mWays, NULL);
}

DLL_EXPORT void traversalWithProgress(void (*callback)(traversalMsg)) {
    traversalWithProgressAndParas(callback, 1, 12);
}
