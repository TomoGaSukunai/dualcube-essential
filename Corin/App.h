//
// Created by Ayaphis on 25-8-8.
//

#ifndef APP_H
#define APP_H
#include <dcommon.h>

typedef struct  CorinAppData{
    int m_X;
    int m_Y;
    SIZE m_Size;
    HINSTANCE m_hInst;
    HDC m_BackgroundDC;
    HBITMAP m_BackgroundBitmap;
    HWND m_hWnd;
}CorinAppData;

LRESULT CALLBACK AppOnCreate(HWND, UINT, WPARAM, LPARAM, CorinAppData*);
LRESULT CALLBACK AppOnEraseBackGround(HWND, UINT, WPARAM, LPARAM, CorinAppData*);

#endif //APP_H
