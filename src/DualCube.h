
#ifndef DUALCUBE_H
#define DUALCUBE_H

#ifdef __WIN32__
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif // DLL_EXPORT
DLL_EXPORT void traversal();

#endif // DUALCUBE_H