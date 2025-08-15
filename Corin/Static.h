//
// Created by Ayaphis on 25-8-10.
//

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H
typedef struct CorinProgressBarData {
    int m_X;
    int m_Y;
    SIZE m_Size;
    HWND m_hWnd;
    HDC m_BackgroundDC;
    BYTE m_Alpha;
    COLORREF m_GlassColor;
    BOOL m_MouseHover;
    BOOL m_TrackNow;
    UINT_PTR m_HoverTimer;
    float m_Ratio;
    BOOL m_Meter;
    COLORREF m_TextColor;
    // void* m_App;
} CorinStaticData;

CorinStaticData CreateCorinStatic(int, int, int, int, HWND, HMENU, HINSTANCE, LPWSTR);
LRESULT CALLBACK StaticOnDrawItem(HWND, UINT, WPARAM, LPARAM, const CorinStaticData*);
LRESULT CALLBACK StaticOnDrawItem2(HWND, UINT, WPARAM, LPARAM, const CorinStaticData*, const char logs[4096]);
#endif //PROGRESSBAR_H
