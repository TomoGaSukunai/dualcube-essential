// #define UNICODE

#include <math.h>
#include <pthread.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Corin.h"
#include "DualCube.h"
#include "post.h"
#include "getSysInfo.h"

#define IDC_BN_SINGLE 0x0100
#define IDC_BN_SUBMIT 0x0101


#define IDC_SS_MAC_MD5 0x0111
#define IDC_SS_HOSTNAME 0x0112
#define IDC_SS_OSNAME 0x0113
#define IDC_SS_CPU_BRAND 0x0114
#define IDC_SS_TIME_MS 0x0115

#define IDC_LB_MD5 0x0121
#define IDC_LB_HOSTNAME 0x0122
#define IDC_LB_OSNAME 0x0123
#define IDC_LB_CPU_BRAND 0x0124
#define IDC_LB_TIME_MS 0x0125


#define IDC_PB_RUNNING 0x0130


#define VERSION "GUI.0.1.0"


CorinAppData appData;

CorinButtonData buttonSingle;
CorinButtonData buttonSubmit;

CorinStaticData labelMacMD5;
CorinStaticData labelHostName;
CorinStaticData labelOSName;
CorinStaticData labelCPUBrand;
CorinStaticData labelTimeMs;

CorinStaticData staticMacMD5;
CorinStaticData staticHostName;
CorinStaticData staticOSName;
CorinStaticData staticCPUBrand;
CorinStaticData staticTimeMs;

CorinStaticData progressRunning;

BOOL run = FALSE;
// const TCHAR labels[][] = {
//     L"Machine id MD5:",
//     L"Hostname:",
//     L"OS Name:"
//     L"CPU Brand:"
//     L"Time take:"
// };

SysInfo info;
struct timespec result_time;



void updateProgress(const float f) {
    SendMessage(appData.m_hWnd, WM_USER, (WPARAM)(long)(floor(f * 100)), (LPARAM)0);
}

typedef struct  {
    void (*callback)(float f);
} updateProgressStruct;

updateProgressStruct ssss;


void traversalWrapper(void* argData) {

    // if (AllocConsole()) {
    //     FILE *pCout;
    //     freopen_s(&pCout, "CONOUT$", "w", stdout);
    //     SetConsoleTitle("TRAVERLING...");
    // }
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    traversalWithProgress(((updateProgressStruct *)argData)->callback);
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    long long milliseconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) /
                             1000000;
    result_time.tv_sec = end.tv_sec - start.tv_sec;
    result_time.tv_nsec = end.tv_nsec - start.tv_nsec;
    if (result_time.tv_nsec < 0) {
        result_time.tv_sec--;
        result_time.tv_nsec += 1000000000L;
    }
    // message box tell time taken by device traversal
    char message[256];
    sprintf(message, "Traversal completed.\nTime taken: %lld ms", milliseconds);
    HWND hEdit = staticTimeMs.m_hWnd;
    char res[128];
    sprintf(res, "%lld ms", milliseconds);

    SetWindowText(hEdit, res);


    // FreeConsole();
    MessageBox(appData.m_hWnd, message, "Traversal Complete", MB_OK | MB_ICONINFORMATION);

    // Resume button and Text Start
    EnableWindow(buttonSingle.m_hWnd, TRUE);
    SetWindowText(buttonSingle.m_hWnd, "Start");
    EnableWindow(buttonSubmit.m_hWnd, TRUE);
    run = FALSE;
    InvalidateRect(hEdit, NULL, TRUE);

}

LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_USER: {
            progressRunning.m_Ratio = (float)wParam/100.0f;
            InvalidateRect(progressRunning.m_hWnd, NULL, TRUE);
            return FALSE;
        }
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT) lParam)->hInstance;
            labelMacMD5 = CreateCorinStatic(10, 10, 90, 30, hwnd, (HMENU) IDC_LB_MD5, hInst, L"mac MD5:");
            labelHostName = CreateCorinStatic(10, 50, 90, 30, hwnd, (HMENU) IDC_LB_HOSTNAME, hInst, L"Host name:");
            labelOSName = CreateCorinStatic(10, 90, 90, 30, hwnd, (HMENU) IDC_LB_OSNAME, hInst, L"OS name:");
            labelCPUBrand = CreateCorinStatic(10, 130, 90, 30, hwnd, (HMENU) IDC_LB_CPU_BRAND, hInst, L"CPUBrand:");
            labelTimeMs = CreateCorinStatic(10, 170, 90, 30, hwnd, (HMENU) IDC_LB_TIME_MS, hInst, L"Time MS:");

            staticMacMD5 = CreateCorinStatic(100, 10, 290, 30, hwnd, (HMENU) IDC_SS_MAC_MD5, hInst, L"");
            staticHostName = CreateCorinStatic(100, 50, 290, 30, hwnd, (HMENU) IDC_SS_HOSTNAME, hInst, L"");
            staticOSName = CreateCorinStatic(100, 90, 290, 30, hwnd, (HMENU) IDC_SS_OSNAME, hInst, L"");
            staticCPUBrand = CreateCorinStatic(100, 130, 290, 30, hwnd, (HMENU) IDC_SS_CPU_BRAND, hInst, L"");
            staticTimeMs = CreateCorinStatic(100, 170, 200, 30, hwnd, (HMENU) IDC_SS_TIME_MS, hInst, L"");

            progressRunning = CreateCorinStatic(100, 170, 200, 30, hwnd, (HMENU) IDC_PB_RUNNING, hInst, L"");
            progressRunning.m_GlassColor = RGB(192, 127, 192);
            progressRunning.m_Alpha = 192;
            progressRunning.m_Meter = TRUE;
            buttonSubmit = CreateCorinButton(300, 170, 90, 30, hwnd, (HMENU) IDC_BN_SUBMIT, hInst);
            buttonSingle = CreateCorinButton(50, 220, 300, 60, hwnd, (HMENU) IDC_BN_SINGLE, hInst);

            info = GetSysInfo();

            SetWindowText(staticMacMD5.m_hWnd, info.machine_id);
            SetWindowText(staticHostName.m_hWnd, info.hostname);
            SetWindowText(staticOSName.m_hWnd, info.os_name);
            SetWindowText(staticCPUBrand.m_hWnd, info.cpu_brand);
            SetWindowText(staticTimeMs.m_hWnd, "Not tested");

            SetWindowText(buttonSingle.m_hWnd, "Single");
            SetWindowText(buttonSubmit.m_hWnd, "Submit");


            return 0;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_BN_SINGLE:
                    run = TRUE;
                    // set button disabled and text running
                    HWND button = buttonSingle.m_hWnd;
                    EnableWindow(button, FALSE);
                    SetWindowText(button, "Running...");
                    InvalidateRect(button, NULL, TRUE);
                    ssss.callback = updateProgress;
                    pthread_t t;
                    pthread_create(&t, NULL, traversalWrapper, (void*)&ssss);
                    break;

                case IDC_BN_SUBMIT:
                    char *link = NULL;
                    postmark(build_json(
                                 info.machine_id,
                                 info.hostname,
                                 info.os_name,
                                 info.cpu_brand,
                                 result_time.tv_sec,
                                 result_time.tv_nsec,
                                 VERSION),
                             &link);
                    if (link != NULL) {
                        char command[256];
                        snprintf(command, 256, "start %s", link);
                        system(command);
                        free(link);
                    }
                    break;
                default: ;
            }
        }
            return 0;
        case WM_DRAWITEM: {
            switch (LOWORD(wParam)) {
                case IDC_BN_SINGLE:
                    return ButtonOnDrawItem(hwnd, uMsg, wParam, lParam, &buttonSingle);
                case IDC_BN_SUBMIT:
                    return ButtonOnDrawItem(hwnd, uMsg, wParam, lParam, &buttonSubmit);
                case IDC_SS_MAC_MD5:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &staticMacMD5);
                case IDC_SS_HOSTNAME:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &staticHostName);
                case IDC_SS_OSNAME:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &staticOSName);
                case IDC_SS_CPU_BRAND:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &staticCPUBrand);
                case IDC_SS_TIME_MS:
                    return !run && StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &staticTimeMs);
                case IDC_PB_RUNNING:
                    return run && StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &progressRunning);
                case IDC_LB_MD5:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &labelMacMD5);
                case IDC_LB_HOSTNAME:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &labelHostName);
                case IDC_LB_OSNAME:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &labelOSName);
                case IDC_LB_CPU_BRAND:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &labelCPUBrand);
                case IDC_LB_TIME_MS:
                    return StaticOnDrawItem(hwnd, uMsg, wParam, lParam, &labelTimeMs);
                default:
                    return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
        }
            return 0;
        default: ;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

const char CLASS_NAME[] = "DualCubeApp";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize COM
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        MessageBox(NULL, "COM initialization failed", "Error", MB_ICONERROR);
        return 0;
    }

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = MainProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm = NULL;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Failed to register window class", "Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        CLASS_NAME,
        "Doki Doki Dual Cube",

        // Remove thick frame and maximize box
        DS_SETFONT | DS_MODALFRAME | WS_MINIMIZEBOX | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,

        // Position and size of the window
        CW_USEDEFAULT, CW_USEDEFAULT,
        420, 320,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Failed to create window", "Error", MB_ICONERROR);
        return 0;
    }
    appData.m_hWnd = hwnd;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();

    return (int) msg.wParam;
}
