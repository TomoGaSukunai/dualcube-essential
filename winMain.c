
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/DualCube.h"
#include "src/post.h"
#include "src/getSysInfo.h"

#define IDB_START 2001
#define IDB_SUBMIT 2002
#define ID_EDIT_MAC 3001
#define ID_EDIT_HOST 3002
#define ID_EDIT_OS 3003
#define ID_EDIT_CPU 3004
#define ID_EDIT_TIME 3005

#define VERSION "GUI.0.1.0"

sysInfo info;
struct timespec result_time;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Initialize infos
        char *labels[] = {
            "MachineId:",
            "Hostname:",
            "OS name:",
            "CPU brand:",
            "Time take:",
            NULL};
        HMENU boxes[] = {
            (HMENU)ID_EDIT_MAC,
            (HMENU)ID_EDIT_HOST,
            (HMENU)ID_EDIT_OS,
            (HMENU)ID_EDIT_CPU,
            (HMENU)ID_EDIT_TIME};

        for (int i = 0; labels[i] != NULL; i++)
        {
            // label
            CreateWindowEx(
                0,
                "STATIC",
                labels[i],
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                10, 10 + 40 * i, 90, 20,
                hwnd,
                NULL,
                ((LPCREATESTRUCT)lParam)->hInstance,
                NULL);
            // box

            // Result Box
            CreateWindowEx(
                0,
                "EDIT",
                "N/A",
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                100, 10 + 40 * i, 290, 20,
                hwnd,
                boxes[i],
                ((LPCREATESTRUCT)lParam)->hInstance,
                NULL);
        }

        info = getSysInfo();
        SetWindowText(GetDlgItem(hwnd, ID_EDIT_MAC), info.machine_id);
        SetWindowText(GetDlgItem(hwnd, ID_EDIT_HOST), info.hostname);
        SetWindowText(GetDlgItem(hwnd, ID_EDIT_OS), info.os_name);
        SetWindowText(GetDlgItem(hwnd, ID_EDIT_CPU), info.cpu_brand);
        SetWindowText(GetDlgItem(hwnd, ID_EDIT_TIME), "Not tested");

        CreateWindowEx(
            0,
            "BUTTON",
            "Submit",
            WS_TABSTOP | WS_CHILD | WS_DISABLED | WS_VISIBLE,
            260, 190, 100, 20,
            hwnd,
            (HMENU)IDB_SUBMIT,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL);

        // Buton to start traversal
        CreateWindowEx(
            0,
            "BUTTON",
            "Start",
            WS_TABSTOP | WS_CHILD | WS_VISIBLE,
            10, 210, 380, 40,
            hwnd,
            (HMENU)IDB_START,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL);
    }
        return 0;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {

        case IDB_START:
            // set button disabled and text running
            HWND button = GetDlgItem(hwnd, IDB_START);
            EnableWindow(button, FALSE);
            SetWindowText(button, "Running...");
            if (AllocConsole())
            {
                FILE *pCout;
                freopen_s(&pCout, "CONOUT$", "w", stdout);
                SetConsoleTitle("TRAVERLING...");
            }
            struct timespec start;
            clock_gettime(CLOCK_MONOTONIC, &start);
            traversal();
            struct timespec end;
            clock_gettime(CLOCK_MONOTONIC, &end);
            long milliseconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
            result_time.tv_sec = end.tv_sec - start.tv_sec;
            result_time.tv_nsec = end.tv_nsec - start.tv_nsec;
            if (result_time.tv_nsec < 0)
            {
                result_time.tv_sec--;
                result_time.tv_nsec += 1000000000L;
            }
            // message box tell time taken by device traversal
            char message[256];
            sprintf(message, "Traversal completed.\nTime taken: %ld ms", milliseconds);
            HWND hEdit = GetDlgItem(hwnd, ID_EDIT_TIME);
            char res[128];
            sprintf(res, "%dms", milliseconds);

            SetWindowText(hEdit, res);
            FreeConsole();
            MessageBox(hwnd, message, "Traversal Complete", MB_OK | MB_ICONINFORMATION);

            // Resume button and Text Start
            EnableWindow(button, TRUE);
            SetWindowText(button, "Start");
            EnableWindow(GetDlgItem(hwnd, IDB_SUBMIT), TRUE);
            break;

        case IDB_SUBMIT:
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
            if (link != NULL)
            {
                char command[256];
                snprintf(command, 256, "start %s", link);
                system(command);
                free(link);
            }
            break;
        }
    }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize COM
    HRESULT hrCOM = CoInitialize(NULL);
    if (FAILED(hrCOM))
    {
        MessageBox(NULL, "COM initialization failed", "Error", MB_ICONERROR);
        return 0;
    }

    const char CLASS_NAME[] = "Sample Window Class";
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm = NULL;

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Failed to register window class", "Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        CLASS_NAME,
        "Not Doki Cube OpenCL",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, // Remove thick frame and maximize box

        // Position and size of the window
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,

        NULL,
        NULL,
        hInstance,
        NULL);
    if (hwnd == NULL)
    {
        MessageBox(NULL, "Failed to create window", "Error", MB_ICONERROR);
        return 0;
    }
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Uninitialize COM
    CoUninitialize();

    return msg.wParam;
}
