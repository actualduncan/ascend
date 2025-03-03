#include "DX12_Helpers.h"
void VerifyD3D12Result(HRESULT D3DResult, const char* code, const char* Filename, INT32 Line)
{
    if (FAILED(D3DResult))
    {
        char message[60];
        sprintf(message, "D3D12 ERROR: %s failed at %s:%u\nWith the ERROR %08X \n", code, Filename, Line, (INT32)D3DResult);
        OutputDebugStringA(message);
        exit(0);
    }
}

std::wstring GetShader(LPCWSTR shaderFile)
{
    std::wstring shaderPath = L"/Shader/";
    WCHAR path[512];
    shaderPath += shaderFile;
    GetAssetPath(path, _countof(path));
    std::wstring assetPaths = path;
    shaderPath = assetPaths + shaderPath;

    return shaderPath;
}

std::wstring s2ws(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}