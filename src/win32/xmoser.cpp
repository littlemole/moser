﻿
#include "pch.h"
#include "xmoser.h"
#include "win32/uni.h"

extern "C" int main(int argc, char** argv);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    bool hasConsole = false;
    if (__argc == 1) // REPL mode
    {
        ::AllocConsole();
        hasConsole = true;
    }
    else 
    { 
        hasConsole = AttachConsole(ATTACH_PARENT_PROCESS);
    }

    if (hasConsole)
    {
        FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
        freopen_s(&fpstdin, "CONIN$", "r", stdin);
        freopen_s(&fpstderr, "CONOUT$", "w", stderr);
        freopen_s(&fpstdout, "CONOUT$", "w", stdout);
    }
    std::vector<std::string> sargv;
    for (int i = 0; i < __argc; i++)
    {
        std::wstring ws(__wargv[i]);
        std::string s = to_string(ws, CP_UTF8);
        sargv.push_back(s);
    }
    std::vector<char*> cargv;
    for (int i = 0; i < __argc; i++)
    {
        cargv.push_back((char*)sargv[i].c_str());
    }
    int r = main(__argc, &cargv[0]);

    if(hasConsole)
    
        ::FreeConsole();
    return r;
}

//#include "MoserX.h"


//m:mox.Event='Click'
/*
const wchar_t* my_xaml = L""
"<Page Name='thePage' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:m='using:moxaml'>"
"   <Grid Name='theGrid'>"
"    <Image m:mox.Event='Click' Name='theImage' Source='https://littlemole.neocities.org/uioo.jpg' />"
"   </Grid>"
"</Page>"; 

// Forward declarations of functions included in this code module:
void                MyRegisterClass(HINSTANCE hInstance, const wchar_t* szWindowClass);
HWND                InitInstance(HINSTANCE, int, const wchar_t* szTitle, const wchar_t* szWindowClass);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool                ProcessMessageForTabNavigation(const HWND topLevelWindow, MSG* msg);

MoserX xmos;



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    try
    {
        // Island-support: Call init_apartment to initialize COM and WinRT for the thread.

        xmos.init();

        // The title bar text
        WCHAR szTitle[100];
        LoadStringW(hInstance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));

        // The main window class name
        WCHAR szWindowClass[100];
        LoadStringW(hInstance, IDC_XMOSER, szWindowClass, ARRAYSIZE(szWindowClass));

        MyRegisterClass(hInstance, szWindowClass);

        // Perform application initialization:
        InitInstance(hInstance, nCmdShow, szTitle, szWindowClass);

        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_XMOSER));

        MSG msg{};

        // Main message loop:
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            // Island-support: It's important to call ContentPreTranslateMessage in the event loop so that WinAppSDK can be aware of
            // the messages.  If you don't need to use an accelerator table, you could just call DispatcherQueue.RunEventLoop
            // to do the message pump for you (it will call ContentPreTranslateMessage automatically).
            if(xmos.translate(msg))
            {
                continue;
            }

            if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Island-support: To properly shut down after using a DispatcherQueue, call ShutdownQueue[Aysnc]().
       // dispatcherQueueController.ShutdownQueue();
        xmos.shutdown();
    }
    catch (const winrt::hresult_error& exception)
    {
        // An exception was thrown, let's make the exit code the HR value of the exception.
        return exception.code().value;
    }

    return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
void MyRegisterClass(HINSTANCE hInstance, const wchar_t* szWindowClass)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XMOSER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_XMOSER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow, const wchar_t* szTitle, const wchar_t* szWindowClass)
{
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, ::GetModuleHandle(NULL), nullptr);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            xmos.create(hWnd);
            const HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            ::CreateWindow(L"BUTTON", L"Win32 Button 1", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 10, 10, 150, 40, hWnd, (HMENU)501, hInst, NULL);

            xmos.load(hWnd, my_xaml);

            ::CreateWindow(L"BUTTON", L"Win32 Button 2", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 10, 400, 150, 40, hWnd, (HMENU)502, hInst, NULL);

        }
        break;
    case WM_SIZE:
        {
            const int width = LOWORD(lParam);
            const int height = HIWORD(lParam);

            ::SetWindowPos(::GetDlgItem(hWnd, 501), NULL, 10, 10, 150, 40, SWP_NOZORDER);
            ::SetWindowPos(::GetDlgItem(hWnd, 502), NULL, 10, height - 50, 150, 40, SWP_NOZORDER);

            RECT r{ 10, 60, width - 20, height - 120 };
            xmos.size(hWnd, r);
        }
        break;
    case WM_ACTIVATE:
        {
            // Make focus work nicely when the user presses alt+tab to activate a different window, and then alt+tab
            // again to come back to this window.  We want the focus to go back to the same child HWND that was focused
            // before.
            xmos.activate(hWnd, wParam);
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            UNREFERENCED_PARAMETER(hdc);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        break;
    case WM_NCDESTROY:

        xmos.destroy(hWnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

*/

