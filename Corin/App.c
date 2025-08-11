//
// Created by Ayaphis on 25-8-8.
//
#include <windows.h>
#include "App.h"


LRESULT CALLBACK AppOnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, CorinAppData* app) {
    return FALSE;
}


LRESULT CALLBACK AppOnEraseBackGround(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, CorinAppData* app) {
    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rc;
    GetClientRect(hWnd, &rc);

    if (app->m_BackgroundDC == NULL && app->m_BackgroundBitmap != NULL) {

        HDC hdcMem = CreateCompatibleDC(hdc);

        // Create from hdc to avoid monocolor failure
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);

        // HBITMAP oldBitmap =
            SelectObject(hdcMem, hBitmap);

        HBRUSH hBrush = CreateSolidBrush(RGB(215, 215, 215));
        FillRect(hdcMem, &rc, hBrush);
        DeleteObject(hBrush);

        hBrush = CreateSolidBrush(RGB(200, 215, 189));

        void** old = SelectObject(hdcMem, hBrush);
        BeginPath(hdcMem);
        MoveToEx(hdcMem, 0,0 , NULL);
        LineTo(hdcMem, 0, 300);
        LineTo(hdcMem, 200,0);
        LineTo(hdcMem, 0, 0);
        EndPath(hdcMem);
        FillPath(hdcMem);


        SelectObject(hdcMem,old);
        DeleteObject(hBrush);

        BITMAP srcBmp, dstBmp;
        GetObject(app->m_BackgroundBitmap, sizeof(BITMAP), &srcBmp);
        GetObject(hBitmap, sizeof(BITMAP), &dstBmp);
        LONG lenSrc = (LONG)(srcBmp.bmWidth * srcBmp.bmHeight * sizeof(BITMAP) * 4);
        BYTE *srcBuffer = (BYTE *) malloc(lenSrc);
        GetBitmapBits(app->m_BackgroundBitmap, lenSrc, srcBuffer);

        // BITMAPINFO bmi = {
        //     .bmiHeader = {
        //         .biSize = sizeof(BITMAPINFO),
        //         .biWidth = dstBmp.bmWidth,
        //         .biHeight = -dstBmp.bmHeight,
        //         .biPlanes = 1,
        //         .biBitCount = 32,
        //         .biCompression = BI_RGB,
        //     },
        // };

        // GetDIBits(hdcMem, hBitmap, 0, dstBmp.bmHeight, NULL, &bmi, DIB_RGB_COLORS);
        // BYTE *dstBuffer = malloc(bmi.bmiHeader.biSizeImage);
        // GetDIBits(hdcMem, hBitmap, 0, dstBmp.bmHeight, dstBuffer, &bmi, DIB_RGB_COLORS);
        LONG lenDst = (LONG)(dstBmp.bmWidth * dstBmp.bmHeight * 4);
        BYTE* dstBuffer = malloc(lenDst);
        GetBitmapBits(hBitmap, lenDst, dstBuffer);

        for (size_t y = 0; y < dstBmp.bmHeight; y++) {
            for (size_t x = 0; x < dstBmp.bmWidth; x++) {
                size_t nSrc = (x + y * srcBmp.bmWidth) * sizeof(BYTE) * 4;
                if (nSrc >= lenSrc) continue;
                size_t nDst = (x + y * dstBmp.bmWidth) * sizeof(BYTE) * 4;
                if (nDst >= lenDst) continue;
                unsigned int alpha = srcBuffer[nSrc + 3];
                unsigned int na = 255 - alpha;
                dstBuffer[nDst + 0] = (srcBuffer[nSrc + 0] * alpha + dstBuffer[nDst + 0] * na) / 255;
                dstBuffer[nDst + 1] = (srcBuffer[nSrc + 1] * alpha + dstBuffer[nDst + 1] * na) / 255;
                dstBuffer[nDst + 2] = (srcBuffer[nSrc + 2] * alpha + dstBuffer[nDst + 2] * na) / 255;
                dstBuffer[nDst + 3] = 255;
            }
        }

        SetBitmapBits(hBitmap, lenDst, dstBuffer);
        // SetDIBits(hdcMem, hBitmap, 0, dstBmp.bmHeight, dstBuffer, &bmi,DIB_RGB_COLORS);
        free(dstBuffer);
        free(srcBuffer);

        app->m_BackgroundDC = hdcMem;
        // app.m_BackgroundDC = CreateCompatibleDC(hdc);
        // BitBlt(app.m_BackgroundDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdcMem, 0, 0,SRCCOPY);

        // SelectObject(hdcMem, oldBitmap);
        // DeleteDC(hdcMem);
    }

    BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, app->m_BackgroundDC, 0, 0,SRCCOPY);

    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    return FALSE;
}
