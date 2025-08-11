
#ifndef DUALCUBE_H
#define DUALCUBE_H

#ifdef __WIN32__
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif // DLL_EXPORT

#define TRAVERSAL_MSG_STEP 1
#define TRAVERSAL_MSG_INFO 2

typedef struct traversalMsg {
    char msgType;
    union {
        int step[2];
        char str[254];
    } msgData;
}traversalMsg;

DLL_EXPORT void traversal();
DLL_EXPORT void traversalWithProgress( void (*callback)(traversalMsg));
#endif // DUALCUBE_H