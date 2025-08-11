//
// Created by Ayaphis on 25-8-10.
//
#define UNICODE
#include <windows.h>
#include <commctrl.h>

#include <wchar.h>
#include "Static.h"


CorinStaticData CreateCorinStatic(int x, int y, int w, int h, HWND hWnd, HMENU hMenu, HINSTANCE hInstance, LPWSTR text) {
    CorinStaticData data;
    data.m_X = x;
    data.m_Y = y;
    data.m_Size.cx = w;
    data.m_Size.cy = h;
    data.m_hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"Static",
        text,
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        data.m_X,
        data.m_Y,
        data.m_Size.cx,
        data.m_Size.cy,

        hWnd,
        hMenu,
        hInstance,
        NULL
        );
    data.m_GlassColor = RGB(255,255,2555);
    data.m_Meter = FALSE;
    data.m_Alpha = 192;
    data.m_Ratio = 1.0f;
    return data;


}
LRESULT CALLBACK StaticOnDrawItem(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,const CorinStaticData* data) {
    LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT) lParam;
    HDC hdc = lpDis -> hDC;
    RECT rc = lpDis -> rcItem;

    HDC hdcMem = CreateCompatibleDC(hdc);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, data->m_Size.cx, data->m_Size.cy);

    HBITMAP oldBitmap = SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, data->m_BackgroundDC,data->m_X,data->m_Y,SRCCOPY);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);
    LONG _size = (LONG)(bmp.bmWidth * bmp.bmHeight * sizeof(BYTE) * 4);
    BYTE* pBits = malloc(_size);
    GetBitmapBits(hBitmap, _size, pBits);

    BYTE r = GetRValue(data->m_GlassColor);
    BYTE g = GetGValue(data->m_GlassColor);
    BYTE b = GetBValue(data->m_GlassColor);
    BYTE a = data->m_Alpha;
    BYTE na = 255 - a;

    int alterx = bmp.bmWidth;
    if (data->m_Meter)
        alterx = (int)( alterx * min(data->m_Ratio, 1.0));


    for (int y = 0; y < bmp.bmHeight; y++) {
        //
        for (int x = 0; x < alterx; x++) {
            int p = (y * bmp.bmWidth + x) * 4;
            pBits[p + 0] = (pBits[p + 0] * na + b * a)/255;
            pBits[p + 1] = (pBits[p + 1] * na + g * a)/255;
            pBits[p + 2] = (pBits[p + 2] * na + r * a)/255;
            // pBits[p + 3] = 255;
        }
        //
        for (int x = alterx; x < bmp.bmWidth; x++) {
            int p = (y * bmp.bmWidth + x) * 4;
            pBits[p + 0] = (pBits[p + 0] * a + b * na)/255;
            pBits[p + 1] = (pBits[p + 1] * a + g * na)/255;
            pBits[p + 2] = (pBits[p + 2] * a + r * na)/255;
            // pBits[p + 3] = 255;
        }
    }
    SetBitmapBits(hBitmap, _size, pBits);
    free(pBits);

    BitBlt(hdc, rc.left, rc.top, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);

    TCHAR text[256];
    int len = 0;
    // SendMessage(lpDis->hwndItem,
    //     CB_GETLBTEXT,
    //     lpDis->itemID,
    //     text);
    if (data->m_Meter) {
        int percentage = (int)(100 * data->m_Ratio);
        len = _snwprintf(text,256,L"%d%%", percentage);
    }else {
        len = GetWindowText(data->m_hWnd, text, 256);
    }

    LOGFONT lf;
    ZeroMemory(&lf, sizeof(LOGFONT));
    lf.lfHeight = 20;
    lf.lfWeight = FW_NORMAL;
    lstrcpy(lf.lfFaceName, L"Segoe UI");
    SendMessage(hWnd, WM_SETFONT, (WPARAM) CreateFontIndirect(&lf), TRUE);
    HFONT hFont = CreateFontIndirect(&lf);
    SelectObject(hdc, hFont);
    HFONT oldFont = SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    DrawText(hdc, text, len ,&rc ,
    DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);

    return FALSE;
}


LRESULT CALLBACK StaticOnDrawItem2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,const CorinStaticData* data, const char logs[4096]) {
    LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT) lParam;
    HDC hdc = lpDis -> hDC;
    RECT rc = lpDis -> rcItem;

    HDC hdcMem = CreateCompatibleDC(hdc);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, data->m_Size.cx, data->m_Size.cy);

    HBITMAP oldBitmap = SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, data->m_BackgroundDC,data->m_X,data->m_Y,SRCCOPY);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);
    LONG _size = (LONG)(bmp.bmWidth * bmp.bmHeight * sizeof(BYTE) * 4);
    BYTE* pBits = malloc(_size);
    GetBitmapBits(hBitmap, _size, pBits);

    BYTE r = GetRValue(data->m_GlassColor);
    BYTE g = GetGValue(data->m_GlassColor);
    BYTE b = GetBValue(data->m_GlassColor);
    BYTE a = data->m_Alpha;
    BYTE na = 255 - a;

    int alterx = bmp.bmWidth;
    if (data->m_Meter)
        alterx = (int)( alterx * min(data->m_Ratio, 1.0));


    for (int y = 0; y < bmp.bmHeight; y++) {
        //
        for (int x = 0; x < alterx; x++) {
            int p = (y * bmp.bmWidth + x) * 4;
            pBits[p + 0] = (pBits[p + 0] * na + b * a)/255;
            pBits[p + 1] = (pBits[p + 1] * na + g * a)/255;
            pBits[p + 2] = (pBits[p + 2] * na + r * a)/255;
            // pBits[p + 3] = 255;
        }
        //
        for (int x = alterx; x < bmp.bmWidth; x++) {
            int p = (y * bmp.bmWidth + x) * 4;
            pBits[p + 0] = (pBits[p + 0] * a + b * na)/255;
            pBits[p + 1] = (pBits[p + 1] * a + g * na)/255;
            pBits[p + 2] = (pBits[p + 2] * a + r * na)/255;
            // pBits[p + 3] = 255;
        }
    }
    SetBitmapBits(hBitmap, _size, pBits);
    free(pBits);

    BitBlt(hdc, rc.left, rc.top, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);



    LOGFONT lf;
    ZeroMemory(&lf, sizeof(LOGFONT));
    lf.lfHeight = 24;
    lf.lfWeight = FW_NORMAL;
    lstrcpy(lf.lfFaceName, L"Segoe UI");
    SendMessage(hWnd, WM_SETFONT, (WPARAM) CreateFontIndirect(&lf), TRUE);
    HFONT hFont = CreateFontIndirect(&lf);
    SelectObject(hdc, hFont);
    HFONT oldFont = SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(100, 115, 89));
    SetBkMode(hdc, TRANSPARENT);

    DrawTextA(hdc, logs, 4096 ,&rc ,
    DT_LEFT | DT_TOP | DT_WORDBREAK);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);

    return FALSE;
}