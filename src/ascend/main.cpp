#include <iostream>
#include <d3d12.h>
#include <wrl.h>
#include "WinUser.h"
#include "Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
static HWND hwnd = NULL;
RenderDevice* Device = nullptr;
bool CreateWindowsApplication(int wHeight, int wWidth, HINSTANCE hInstance, int nCmdShow);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    if(CreateWindowsApplication(1280, 800, hInstance, nCmdShow))
    {
       // IMGUI_CHECKVERSION();
       // ImGui::CreateContext();
       // ImGuiIO& io = ImGui::GetIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
       // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
       // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

       // ImGui_ImplWin32_Init(hwnd);
        Device = new RenderDevice();
        Device->Initialize(hwnd);

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

            Device->OnRender();
        }
        return 0;

        // Return this part of the WM_QUIT message to Windows.
        
    }
   
    return 0;
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        
        break;
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