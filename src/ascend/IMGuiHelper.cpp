#include "IMGuiHelper.h"
#include "DX12/GraphicsTypes.h"
#include "DX12/DX12_Helpers.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "Shader/CompiledShaders/IMGuiPS.hlsl.h"
#include "Shader/CompiledShaders/IMGuiVS.hlsl.h"

IMGUI_Helper::IMGUI_Helper(HWND hwnd)
{
    m_hwnd = hwnd;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    m_imguiStyle = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        m_imguiStyle.WindowRounding = 0.0f;
        m_imguiStyle.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    io.Fonts->AddFontDefault();

}

IMGUI_Helper::~IMGUI_Helper()
{

}

void IMGUI_Helper::InitWin32DX12()
{
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_hwnd);

    DescriptorAllocation alloc = DX12::UAVDescriptorHeap.AllocateDescriptor();

    ImGui_ImplDX12_Init(DX12::Device.Get(), DX12::RenderLatency,
        DXGI_FORMAT_R8G8B8A8_UNORM, DX12::UAVDescriptorHeap.Heap.Get(),
        alloc.Handle,
        alloc.GPUHandle);
}

void IMGUI_Helper::StartFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void IMGUI_Helper::EndFrame()
{

    ImGui::Render();

    DX12::GraphicsCmdList->SetPipelineState(m_pipelineStateObject.Get());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), DX12::GraphicsCmdList.Get());


    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, DX12::GraphicsCmdList.Get());


}

void IMGUI_Helper::CreatePSO()
{
    {
        CD3DX12_DESCRIPTOR_RANGE UAVDescriptor[1] = {};
        UAVDescriptor[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
        rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor[0]);
        rootParameters[1].InitAsConstantBufferView(0);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = 0;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        VERIFYD3D12RESULT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
        VERIFYD3D12RESULT(DX12::Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    {
        D3D12_INPUT_ELEMENT_DESC inputElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElements, _countof(inputElements) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE((void*)g_pIMGuiVS, ARRAYSIZE(g_pIMGuiVS));
        psoDesc.PS = CD3DX12_SHADER_BYTECODE((void*)g_pIMGuiPS, ARRAYSIZE(g_pIMGuiPS));
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.AlphaToCoverageEnable = TRUE;
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        VERIFYD3D12RESULT(DX12::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateObject)));
    }
}