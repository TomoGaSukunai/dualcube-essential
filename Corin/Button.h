//
// Created by Ayaphis on 25-8-8.
//

#ifndef BUTTON_H
#define BUTTON_H

typedef struct CorinButtonData {
    int m_X;
    int m_Y;
    SIZE m_Size;
    HWND m_hWnd;
    BYTE m_Alpha;
    HDC m_BackgroundDC;
    COLORREF m_GlassColor;
    BOOL m_MouseHover;
    BOOL m_TrackNow;
    UINT_PTR m_HoverTimer;
    // void* m_App;
} CorinButtonData;

CorinButtonData CreateCorinButton(int, int, int, int, HWND, HMENU, HINSTANCE);

LRESULT CALLBACK ButtonOnDrawItem(HWND, UINT, WPARAM, LPARAM, const CorinButtonData *);

LRESULT CALLBACK ButtonSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

#endif //BUTTON_H
