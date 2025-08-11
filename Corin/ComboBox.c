//
// Created by Ayaphis on 25-8-5.
//
#include <windows.h>
#include "ComboBox.h"
CorinComboBoxData CreateCorinComboBoxData(int x, int y, int width, int height, HWND hWnd, HMENU hMenu, HINSTANCE hInstance) {
    CorinComboBoxData data;
    data.m_X = x;
    data.m_Y = y;
    data.m_Size.cx = width;
    data.m_Size.cy = height;
    data.m_hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "ComboBox",
        NULL,
        CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        data.m_X, data.m_Y,
        data.m_Size.cx, data.m_Size.cy,
        hWnd,
        hMenu,
        hInstance,
        NULL
        );
    data.m_Alpha = 0;
    // data.m_BackgroundDC = CreateCompatibleDC(NULL);
    data.m_GlassColor = RGB(255, 255, 255);
    return data;
}


LRESULT CALLBACK ComboBoxOnDrawItem(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, const CorinComboBoxData* data) {
    LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT)lParam;

    int skipLine = 0;
    if (!lpDrawItemStruct->itemState & ODS_SELECTED) {
        skipLine = (int)lpDrawItemStruct->itemID * 18 + 26 ;
    }

    HDC hdc = lpDrawItemStruct->hDC;
    RECT rc = lpDrawItemStruct->rcItem;

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
    HBITMAP oldBitmap = SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, 0, 0, rc.right - rc.left, rc.bottom - rc.top, data->m_BackgroundDC, data->m_X + rc.left, data->m_Y + skipLine ,SRCCOPY);


    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);
    const LONG _size = (LONG)(bmp.bmWidth * bmp.bmHeight * sizeof(BYTE) * 4);
    BYTE* pBits = malloc(_size);
    GetBitmapBits(hBitmap, _size, pBits);

    const BYTE r = GetRValue(data->m_GlassColor);
    const BYTE g = GetGValue(data->m_GlassColor);
    const BYTE b = GetBValue(data->m_GlassColor);
    const BYTE a = data->m_Alpha;
    const BYTE na = 255 - a;

    for (int y = 0; y < bmp.bmHeight; y++) {
        for (int x = 0; x < bmp.bmWidth; x++) {
            const int p = (y * bmp.bmWidth + x) * 4;
            // if (p < 0) continue;
            pBits[p + 0] = (pBits[p + 0] * na + b * a)/255;
            pBits[p + 1] = (pBits[p + 1] * na + g * a)/255;
            pBits[p + 2] = (pBits[p + 2] * na + r * a)/255;
            // pBits[p + 3] = 127;
        }
    }
    SetBitmapBits(hBitmap, _size, pBits);
    free(pBits);

    BitBlt(hdc, rc.left, rc.top, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);

    HBRUSH brush = CreateSolidBrush(RGB(255,255,127));
    HBRUSH oldBrush = SelectObject(hdc, brush);
    if (lpDrawItemStruct-> itemState & ODS_SELECTED) {
        if (rc.left != 0) {

            rc.right = rc.left + 5;
            FillRect(hdc, &rc, brush);
            rc = lpDrawItemStruct->rcItem;
        }else {
            // brush = CreateSolidBrush(COLORREF(0,0,222));
            FillRect(hdc, &rc, brush);
        }
    }else {
        // brush = CreateSolidBrush(bkColor);
        // oldBrush = SelectObject(hdc, brush);
        // // FillRect(hdc, &rc2, brush);
    }
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);

    const TCHAR text[256];
    // int len = GetWindowText(data->m_hWnd, text, 256);
    UINT len = SendMessage(lpDrawItemStruct->hwndItem,
        CB_GETLBTEXT,
        lpDrawItemStruct->itemID,
        text);

    LOGFONT lf;
    ZeroMemory(&lf, sizeof(LOGFONT));
    lf.lfHeight = 18;
    lf.lfWeight = FW_NORMAL;
    lstrcpy(lf.lfFaceName, L"Segoe UI");
    SendMessage(hWnd, WM_SETFONT, (WPARAM) CreateFontIndirect(&lf), TRUE);
    HFONT hFont = CreateFontIndirect(&lf);
    SelectObject(hdc, hFont);
    HFONT oldFont = SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    DrawText(hdc, text, (int)len ,&rc ,
    DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);


    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    // ReleaseDC(hWnd, hdc);

    return FALSE;
}