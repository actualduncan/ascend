#include <iostream>
#include <d3d12.h>
#include <wrl.h>
#include "WinUser.h"
#include "WorkGraphsDXR.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "InputCommands.h"

static HWND hwnd = NULL;
WorkGraphsDXR* WorkGraphsDXR_app = nullptr;
InputCommands appInput;
char m_keyArray[256];

bool CreateWindowsApplication(int wHeight, int wWidth, HINSTANCE hInstance, int nCmdShow);
#include <filesystem>
#include <shlobj.h>

static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::filesystem::path pixInstallationPath = programFilesPath;
    pixInstallationPath /= "Microsoft PIX";

    std::wstring newestVersionFound;

    for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
    {
        if (directory_entry.is_directory())
        {
            if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
            {
                newestVersionFound = directory_entry.path().filename().c_str();
            }
        }
    }

    if (newestVersionFound.empty())
    {
        // TODO: Error, no PIX installation found
    }

    return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    if(CreateWindowsApplication(1280, 800, hInstance, nCmdShow))
    {
        IMGUI_CHECKVERSION();
        //ImGui::CreateContext();
        // ImGuiIO& io = ImGui::GetIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        //ImGui_ImplWin32_Init(hwnd);
        // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
// This may happen if the application is launched through the PIX UI. 
        if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
        {
            LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
        }

        WorkGraphsDXR_app = new WorkGraphsDXR();
        WorkGraphsDXR_app->Initialize(hwnd, 1280, 800);

        // Application loop
        bool done = false;
        while (!done)
        {
            MSG msg = {};
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                // Process any messages in the queue.
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;
            WorkGraphsDXR_app->Update(0.1f, &appInput);
            WorkGraphsDXR_app->Render();
        }
        return 0;

        // Return this part of the WM_QUIT message to Windows.
        
    }
   
    return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    appInput.Reset();
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        break;
    case WM_KEYDOWN:
        m_keyArray[wParam] = true;
        break;

    case WM_KEYUP:
        m_keyArray[wParam] = false;
        break;
    case WM_MOUSEMOVE:
        POINT p;
        if (GetCursorPos(&p))
        {
            appInput.mouseX = p.x;
            appInput.mouseY = p.y;
        }
        break;
    case WM_LBUTTONDOWN:
        appInput.mouseLButtonDown = true;
        break;
    case WM_RBUTTONDOWN:
        appInput.mouseRButtonDown = true;
        break;
    case WM_LBUTTONUP:
        appInput.mouseLButtonDown = false;
        break;
    case WM_RBUTTONUP:
        appInput.mouseRButtonDown = false;
    }

    //WASD movement
    if (m_keyArray['W'])
    {
        appInput.horizontalZ = 1.0f;
    }
    if (m_keyArray['S'])
    {
        appInput.horizontalZ = -1.0f;
    }
    if (m_keyArray['A'])
    {
        appInput.horizontalX = -1.0f;
    }
    if (m_keyArray['D'])
    {
        appInput.horizontalX = 1.0f;
    }

    // Up/Down
    if (m_keyArray['E'])
    {
        appInput.vertical = 1.0f;
    }
    if (m_keyArray['Q'])
    {
        appInput.vertical = -1.0f;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool CreateWindowsApplication(int wHeight, int wWidth, HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"ascendClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, wHeight, wWidth };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    hwnd = CreateWindow(
        windowClass.lpszClassName,
        L"ascend.editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,        // We have no parent window.
        nullptr,        // We aren't using menus.
        hInstance,
        nullptr);
    ShowWindow(hwnd, nCmdShow);
    return true;
}