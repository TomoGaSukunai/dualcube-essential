//
// Created by Ayaphis on 25-8-5.
//

#ifndef COMBOBOX_H
#define COMBOBOX_H
typedef struct CorinComboBoxData {
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

}CorinComboBoxData;
CorinComboBoxData CreateCorinComboBoxData(int, int, int, int, HWND, HMENU, HINSTANCE);
LRESULT CALLBACK ComboBoxOnDrawItem(HWND, UINT, WPARAM, LPARAM, const CorinComboBoxData*) ;
#endif //COMBOBOX_H
