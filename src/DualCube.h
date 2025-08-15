#ifndef DUALCUBE_H
#define DUALCUBE_H

#ifdef __WIN32__
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif // DLL_EXPORT

#define TRAVERSAL_MSG_STEP 1
#define TRAVERSAL_MSG_INFO 2

typedef struct {
    char type;
    union {
        int step[3];
        char str[254];
    } data;
} traversalMsg;



/**
 *@warning to be obsolete
 */
DLL_EXPORT void traversalWithProgress(void (*callback)(traversalMsg));

/**
 *@warning to be obsolete
 */
DLL_EXPORT void traversalWithProgressAndParas(void (*callback)(traversalMsg), const unsigned short nThreads, const int mWays);

/**
 * take a traversal within Dual Cube
 *@warning not thread-safe only
 *@param callback the callback function takes traversalMsg
 *@param nThreads assign threads number if 0 use full cores
 *@param mWays 12 for full 6 for quarte
 *@param que pointer to store que data out
 *@return 0 for success
 */

DLL_EXPORT int traversalDualCube(void (*callback)(traversalMsg), const unsigned short nThreads, const int mWays,
                                 const void **que);

#endif // DUALCUBE_H
