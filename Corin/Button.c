//
// Created by Ayaphis on 25-8-8.
//
#define UNICODE
#include <windows.h>
#include <commctrl.h>

#include <tchar.h>
#include "Button.h"

const int BUTTON_HOVER_DELAY = 50;

CorinButtonData CreateCorinButton(int x, int y, int width, int height, HWND hWnd, HMENU hMenu, HINSTANCE hInstance) {
    CorinButtonData data;
    data.m_X = x;
    data.m_Y = y;
    data.m_Size.cx = width;
    data.m_Size.cy = height;
    data.m_hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"Button",
        L"Hello World!",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        data.m_X,
        data.m_Y,
        data.m_Size.cx,
        data.m_Size.cy,

        hWnd,
        hMenu,
        hInstance,
        NULL

    );
    data.m_Alpha = 127;
    data.m_MouseHover = FALSE;
    data.m_TrackNow = FALSE;
    data.m_GlassColor = RGB(255, 255, 255);

    return data;
}

LRESULT CALLBACK ButtonOnDrawItem(HWND hWnd, UINT, WPARAM, LPARAM lParam, const CorinButtonData *data) {
    LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT) lParam;
    HDC hdc = lpDis->hDC;
    RECT rc = lpDis->rcItem;

    HDC hdcMem = CreateCompatibleDC(hdc);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, data->m_Size.cx, data->m_Size.cy);

    HBITMAP oldBitmap = SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, data->m_BackgroundDC, data->m_X, data->m_Y,
           SRCCOPY);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);
    LONG _size = (LONG) (bmp.bmWidth * bmp.bmHeight * sizeof(BYTE) * 4);
    BYTE *pBits = malloc(_size);
    GetBitmapBits(hBitmap, _size, pBits);


    BYTE r = GetRValue(data->m_GlassColor);
    BYTE g = GetGValue(data->m_GlassColor);
    BYTE b = GetBValue(data->m_GlassColor);
    BYTE a = data->m_Alpha;
    BYTE na = 255 - a;

    BOOL pressed = (BOOL) (lpDis->itemState & ODS_SELECTED);

    if (pressed) {
        // HPEN hPen = CreatePen(PS_SOLID, 30, RGB(255-r, 255-g, 255-b));
        //
        // HPEN oldPen = SelectObject(hdcMem, hPen);
        // MoveToEx(hdcMem, rc.left + 30, rc.top + 30, NULL);
        // LineTo(hdcMem, rc.right - 30, rc.bottom - 30);
        // MoveToEx(hdcMem, rc.left + 30, rc.bottom - 30, NULL);
        // LineTo(hdcMem, rc.right - 30, rc.top + 30);
        // SelectObject(hdcMem, oldPen);
        // DeleteObject(hPen);
        na += na / 2;
        a = 255 - na;
    } else if (data->m_MouseHover) {
        // HPEN hPen = CreatePen(PS_SOLID, 30, RGB(255-r, 255-g, 255-b));
        // HPEN oldPen = SelectObject(hdcMem, hPen);
        // HBRUSH oldBrush = SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
        // // Arc(hdcMem, rc.left + 10, rc.top+ 10, rc.right -10, rc.top +10, rc.right - 10, rc.bottom -10, rc.left + 10, rc.bottom -10);
        // Ellipse(hdcMem, rc.left + 30, rc.top + 30, rc.right - 30, rc.bottom - 30);
        // SelectObject(hdcMem, oldBrush);
        // SelectObject(hdcMem, oldPen);
        // DeleteObject(hPen);
        na *= 2;
        a = 255 - na;
    } else {
    }


    for (int y = 0; y < bmp.bmHeight; y++) {
        for (int x = 0; x < bmp.bmWidth; x++) {
            int p = (y * bmp.bmWidth + x) * 4;
            pBits[p + 0] = (pBits[p + 0] * na + b * a) / 255;
            pBits[p + 1] = (pBits[p + 1] * na + g * a) / 255;
            pBits[p + 2] = (pBits[p + 2] * na + r * a) / 255;
            // pBits[p + 3] = 255;
        }
    }
    SetBitmapBits(hBitmap, _size, pBits);
    free(pBits);




    BitBlt(hdc, rc.left, rc.top, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);

    TCHAR text[256];
    int len = GetWindowText(data->m_hWnd, text, 256);
    // SendMessage(lpDis->hwndItem,
    //     CB_GETLBTEXT,
    //     lpDis->itemID,
    //     text);

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

    DrawText(hdc, text, len, &rc,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    // ReleaseDC(hWnd, hdc);

    return FALSE;
}


LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                    UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    CorinButtonData *data = (CorinButtonData *) dwRefData;

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            // 首次鼠标进入按钮区域
            if (!data->m_MouseHover) {
                data->m_MouseHover = TRUE;

                // 设置悬停检测定时器
                data->m_HoverTimer = SetTimer(GetParent(hWnd), 1, BUTTON_HOVER_DELAY, NULL);

                // 设置鼠标跟踪
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
            break;
        case WM_MOUSELEAVE:
            // 鼠标离开按钮区域
            data->m_MouseHover = FALSE;
            KillTimer(GetParent(hWnd), data->m_HoverTimer);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case WM_NCDESTROY:
            // 移除子类化
            RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
            break;
        default:
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
