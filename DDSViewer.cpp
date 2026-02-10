#include "framework.h"
#include "DDSViewer.h"

#include <shellapi.h>
#include <algorithm>
#include <chrono>
#include <array>
#include <string>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#ifdef min
#undef min
#endif 

#ifdef max 
#undef max 
#endif 

using namespace DirectX;
using namespace Microsoft::WRL;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

ViewerApplication::ViewerApplication() :
    mWindowHandle {},
    mWindowClassName {},
    mRunning { false },
    mDevice {},
    mCommandQueue {},
    mSwapChain {},
    mCommandAllocators {},
    mCommandList {},
    mRenderTargets {},
    mRtvHeap {},
    mSrvHeap {},
    mRtvDescriptorSize { 0 },
    mFrameIndex { 0 },
    mFence {},
    mFenceValue { 0 },
    mFenceEvent {},
    mAnalyzer {},
    mUploader {},
    mSettings { DXGI_FORMAT_BC7_UNORM, TEX_FILTER_FANT, true, false, false, false, CompressionQualityLevel::Normal, ChannelViewMode::Rgba, 1.0f },
    mFormatOptions {},
    mSelectedFormatIndex { 0 },
    mSourceTexture {},
    mSourceUpload {},
    mCompressedTexture {},
    mCompressedUpload {},
    mImGuiCpuHandle {},
    mImGuiGpuHandle {},
    mSourceCpuHandle {},
    mSourceGpuHandle {},
    mCompressedCpuHandle {},
    mCompressedGpuHandle {},
    mHasSourceTexture { false },
    mHasCompressedTexture { false },
    mHasPendingDrop { false },
    mIsProcessing { false },
    mHasLastError { false },
    mLastLoadSeconds { 0.0 },
    mLastCompressSeconds { 0.0 },
    mPendingDropPath {} {
}

ViewerApplication::~ViewerApplication() {
    ShutdownImGui();
    ShutdownD3D12();
}

ViewerApplication::ViewerApplication(const ViewerApplication& Other) :
    mWindowHandle { Other.mWindowHandle },
    mWindowClassName {},
    mRunning { Other.mRunning },
    mDevice {},
    mCommandQueue {},
    mSwapChain {},
    mCommandAllocators {},
    mCommandList {},
    mRenderTargets {},
    mRtvHeap {},
    mSrvHeap {},
    mRtvDescriptorSize { Other.mRtvDescriptorSize },
    mFrameIndex { Other.mFrameIndex },
    mFence {},
    mFenceValue { Other.mFenceValue },
    mFenceEvent {},
    mAnalyzer { Other.mAnalyzer },
    mUploader { Other.mUploader },
    mSettings { Other.mSettings },
    mFormatOptions { Other.mFormatOptions },
    mSelectedFormatIndex { Other.mSelectedFormatIndex },
    mSourceTexture {},
    mSourceUpload {},
    mCompressedTexture {},
    mCompressedUpload {},
    mImGuiCpuHandle { Other.mImGuiCpuHandle },
    mImGuiGpuHandle { Other.mImGuiGpuHandle },
    mSourceCpuHandle { Other.mSourceCpuHandle },
    mSourceGpuHandle { Other.mSourceGpuHandle },
    mCompressedCpuHandle { Other.mCompressedCpuHandle },
    mCompressedGpuHandle { Other.mCompressedGpuHandle },
    mHasSourceTexture { Other.mHasSourceTexture },
    mHasCompressedTexture { Other.mHasCompressedTexture },
    mHasPendingDrop { Other.mHasPendingDrop },
    mIsProcessing { Other.mIsProcessing },
    mHasLastError { Other.mHasLastError },
    mLastLoadSeconds { Other.mLastLoadSeconds },
    mLastCompressSeconds { Other.mLastCompressSeconds },
    mPendingDropPath { std::move(Other.mPendingDropPath) } {
    memcpy(mWindowClassName, Other.mWindowClassName, sizeof(mWindowClassName));
}

ViewerApplication& ViewerApplication::operator=(const ViewerApplication& Other) {
    if (this != &Other) {
        mWindowHandle = Other.mWindowHandle;
        memcpy(mWindowClassName, Other.mWindowClassName, sizeof(mWindowClassName));
        mRunning = Other.mRunning;
        mRtvDescriptorSize = Other.mRtvDescriptorSize;
        mFrameIndex = Other.mFrameIndex;
        mFenceValue = Other.mFenceValue;
        mAnalyzer = Other.mAnalyzer;
        mUploader = Other.mUploader;
        mSettings = Other.mSettings;
        mFormatOptions = Other.mFormatOptions;
        mSelectedFormatIndex = Other.mSelectedFormatIndex;
        mImGuiCpuHandle = Other.mImGuiCpuHandle;
        mImGuiGpuHandle = Other.mImGuiGpuHandle;
        mSourceCpuHandle = Other.mSourceCpuHandle;
        mSourceGpuHandle = Other.mSourceGpuHandle;
        mCompressedCpuHandle = Other.mCompressedCpuHandle;
        mCompressedGpuHandle = Other.mCompressedGpuHandle;
        mHasSourceTexture = Other.mHasSourceTexture;
        mHasCompressedTexture = Other.mHasCompressedTexture;
        mHasPendingDrop = Other.mHasPendingDrop;
        mIsProcessing = Other.mIsProcessing;
        mHasLastError = Other.mHasLastError;
        mLastLoadSeconds = Other.mLastLoadSeconds;
        mLastCompressSeconds = Other.mLastCompressSeconds;
        mPendingDropPath = Other.mPendingDropPath;
    }
    return *this;
}

ViewerApplication::ViewerApplication(ViewerApplication&& Other) noexcept :
    mWindowHandle { Other.mWindowHandle },
    mWindowClassName {},
    mRunning { Other.mRunning },
    mDevice { std::move(Other.mDevice) },
    mCommandQueue { std::move(Other.mCommandQueue) },
    mSwapChain { std::move(Other.mSwapChain) },
    mCommandAllocators { std::move(Other.mCommandAllocators) },
    mCommandList { std::move(Other.mCommandList) },
    mRenderTargets { std::move(Other.mRenderTargets) },
    mRtvHeap { std::move(Other.mRtvHeap) },
    mSrvHeap { std::move(Other.mSrvHeap) },
    mRtvDescriptorSize { Other.mRtvDescriptorSize },
    mFrameIndex { Other.mFrameIndex },
    mFence { std::move(Other.mFence) },
    mFenceValue { Other.mFenceValue },
    mFenceEvent { Other.mFenceEvent },
    mAnalyzer { std::move(Other.mAnalyzer) },
    mUploader { std::move(Other.mUploader) },
    mSettings { Other.mSettings },
    mFormatOptions { std::move(Other.mFormatOptions) },
    mSelectedFormatIndex { Other.mSelectedFormatIndex },
    mSourceTexture { std::move(Other.mSourceTexture) },
    mSourceUpload { std::move(Other.mSourceUpload) },
    mCompressedTexture { std::move(Other.mCompressedTexture) },
    mCompressedUpload { std::move(Other.mCompressedUpload) },
    mImGuiCpuHandle { Other.mImGuiCpuHandle },
    mImGuiGpuHandle { Other.mImGuiGpuHandle },
    mSourceCpuHandle { Other.mSourceCpuHandle },
    mSourceGpuHandle { Other.mSourceGpuHandle },
    mCompressedCpuHandle { Other.mCompressedCpuHandle },
    mCompressedGpuHandle { Other.mCompressedGpuHandle },
    mHasSourceTexture { Other.mHasSourceTexture },
    mHasCompressedTexture { Other.mHasCompressedTexture },
    mHasPendingDrop { Other.mHasPendingDrop },
    mIsProcessing { Other.mIsProcessing },
    mHasLastError { Other.mHasLastError },
    mLastLoadSeconds { Other.mLastLoadSeconds },
    mLastCompressSeconds { Other.mLastCompressSeconds },
    mPendingDropPath { std::move(Other.mPendingDropPath) } {
    memcpy(mWindowClassName, Other.mWindowClassName, sizeof(mWindowClassName));
    Other.mWindowHandle = nullptr;
    Other.mFenceEvent = nullptr;
    Other.mHasPendingDrop = false;
}

ViewerApplication& ViewerApplication::operator=(ViewerApplication&& Other) noexcept {
    if (this != &Other) {
        mWindowHandle = Other.mWindowHandle;
        memcpy(mWindowClassName, Other.mWindowClassName, sizeof(mWindowClassName));
        mRunning = Other.mRunning;
        mDevice = std::move(Other.mDevice);
        mCommandQueue = std::move(Other.mCommandQueue);
        mSwapChain = std::move(Other.mSwapChain);
        mCommandAllocators = std::move(Other.mCommandAllocators);
        mCommandList = std::move(Other.mCommandList);
        mRenderTargets = std::move(Other.mRenderTargets);
        mRtvHeap = std::move(Other.mRtvHeap);
        mSrvHeap = std::move(Other.mSrvHeap);
        mRtvDescriptorSize = Other.mRtvDescriptorSize;
        mFrameIndex = Other.mFrameIndex;
        mFence = std::move(Other.mFence);
        mFenceValue = Other.mFenceValue;
        mFenceEvent = Other.mFenceEvent;
        mAnalyzer = std::move(Other.mAnalyzer);
        mUploader = std::move(Other.mUploader);
        mSettings = Other.mSettings;
        mFormatOptions = std::move(Other.mFormatOptions);
        mSelectedFormatIndex = Other.mSelectedFormatIndex;
        mSourceTexture = std::move(Other.mSourceTexture);
        mSourceUpload = std::move(Other.mSourceUpload);
        mCompressedTexture = std::move(Other.mCompressedTexture);
        mCompressedUpload = std::move(Other.mCompressedUpload);
        mImGuiCpuHandle = Other.mImGuiCpuHandle;
        mImGuiGpuHandle = Other.mImGuiGpuHandle;
        mSourceCpuHandle = Other.mSourceCpuHandle;
        mSourceGpuHandle = Other.mSourceGpuHandle;
        mCompressedCpuHandle = Other.mCompressedCpuHandle;
        mCompressedGpuHandle = Other.mCompressedGpuHandle;
        mHasSourceTexture = Other.mHasSourceTexture;
        mHasCompressedTexture = Other.mHasCompressedTexture;
        mHasPendingDrop = Other.mHasPendingDrop;
        mIsProcessing = Other.mIsProcessing;
        mHasLastError = Other.mHasLastError;
        mLastLoadSeconds = Other.mLastLoadSeconds;
        mLastCompressSeconds = Other.mLastCompressSeconds;
        mPendingDropPath = std::move(Other.mPendingDropPath);
        Other.mWindowHandle = nullptr;
        Other.mFenceEvent = nullptr;
        Other.mHasPendingDrop = false;
        Other.mIsProcessing = false;
    }
    return *this;
}

bool ViewerApplication::Initialize(HINSTANCE InstanceHandle, int ShowCommand) {
    mFormatOptions = BuildCompressionCandidateFormats();
    if (!CreateMainWindow(InstanceHandle, ShowCommand)) {
        return false;
    }
    if (!InitializeD3D12()) {
        return false;
    }
    if (!InitializeImGui()) {
        return false;
    }
    return true;
}

int ViewerApplication::Run() {
    MSG Message {};
    mRunning = true;
    while (mRunning) {
        while (PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE)) {
            if (Message.message == WM_QUIT) {
                mRunning = false;
            }
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        if (!mRunning) {
            break;
        }
        ProcessPendingDrop();
        if (!BeginFrame()) {
            continue;
        }
        RenderUi();
        RenderFrame();
        EndFrame();
    }
    WaitForGpu();
    return static_cast<int>(Message.wParam);
}

LRESULT CALLBACK ViewerApplication::StaticWindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
    ViewerApplication* Application { reinterpret_cast<ViewerApplication*>(GetWindowLongPtr(WindowHandle, GWLP_USERDATA)) };
    if (Message == WM_NCCREATE) {
        CREATESTRUCTW* CreateInfo { reinterpret_cast<CREATESTRUCTW*>(LParam) };
        Application = reinterpret_cast<ViewerApplication*>(CreateInfo->lpCreateParams);
        SetWindowLongPtr(WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(Application));
    }
    if (Application != nullptr) {
        return Application->WindowProcedure(WindowHandle, Message, WParam, LParam);
    }
    return DefWindowProc(WindowHandle, Message, WParam, LParam);
}

LRESULT ViewerApplication::WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
    if (ImGui_ImplWin32_WndProcHandler(WindowHandle, Message, WParam, LParam)) {
        return 1;
    }
    if (Message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    if (Message == WM_DROPFILES) {
        HandleDroppedFile(reinterpret_cast<HDROP>(WParam));
        return 0;
    }
    return DefWindowProc(WindowHandle, Message, WParam, LParam);
}

bool ViewerApplication::CreateMainWindow(HINSTANCE InstanceHandle, int ShowCommand) {
    const wchar_t* ClassName { L"DdsViewerWindowClass" };
    wcscpy_s(mWindowClassName, ClassName);
    WNDCLASSEXW WindowClass {};
    WindowClass.cbSize = sizeof(WNDCLASSEXW);
    WindowClass.lpfnWndProc = ViewerApplication::StaticWindowProcedure;
    WindowClass.hInstance = InstanceHandle;
    WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    WindowClass.lpszClassName = mWindowClassName;
    if (RegisterClassExW(&WindowClass) == 0) {
        return false;
    }

    mWindowHandle = CreateWindowW(mWindowClassName, L"Texture Artifact Analyzer & Converter", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900, nullptr, nullptr, InstanceHandle, this);
    if (mWindowHandle == nullptr) {
        return false;
    }
    DragAcceptFiles(mWindowHandle, TRUE);
    ShowWindow(mWindowHandle, ShowCommand);
    UpdateWindow(mWindowHandle);
    return true;
}

bool ViewerApplication::InitializeD3D12() {
    ComPtr<IDXGIFactory4> Factory {};
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(Factory.GetAddressOf())))) {
        return false;
    }
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(mDevice.GetAddressOf())))) {
        return false;
    }

    const D3D12_COMMAND_QUEUE_DESC QueueDesc { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
    if (FAILED(mDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf())))) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc {};
    SwapChainDesc.Width = 0;
    SwapChainDesc.Height = 0;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = FrameCount;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> SwapChain1 {};
    if (FAILED(Factory->CreateSwapChainForHwnd(mCommandQueue.Get(), mWindowHandle, &SwapChainDesc, nullptr, nullptr, SwapChain1.GetAddressOf()))) {
        return false;
    }
    if (FAILED(SwapChain1.As(&mSwapChain))) {
        return false;
    }

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    const D3D12_DESCRIPTOR_HEAP_DESC RtvDesc { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FrameCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
    if (FAILED(mDevice->CreateDescriptorHeap(&RtvDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())))) {
        return false;
    }

    const D3D12_DESCRIPTOR_HEAP_DESC SrvDesc { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    if (FAILED(mDevice->CreateDescriptorHeap(&SrvDesc, IID_PPV_ARGS(mSrvHeap.GetAddressOf())))) {
        return false;
    }

    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle { mRtvHeap->GetCPUDescriptorHandleForHeapStart() };
    for (UINT Index { 0 }; Index < FrameCount; ++Index) {
        if (FAILED(mSwapChain->GetBuffer(Index, IID_PPV_ARGS(mRenderTargets[Index].GetAddressOf())))) {
            return false;
        }
        mDevice->CreateRenderTargetView(mRenderTargets[Index].Get(), nullptr, RtvHandle);
        RtvHandle.ptr += static_cast<SIZE_T>(mRtvDescriptorSize);
        if (FAILED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCommandAllocators[Index].GetAddressOf())))) {
            return false;
        }
    }

    if (FAILED(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())))) {
        return false;
    }
    if (FAILED(mCommandList->Close())) {
        return false;
    }

    if (FAILED(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.GetAddressOf())))) {
        return false;
    }
    mFenceValue = 1;
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr) {
        return false;
    }

    const UINT SrvStep { mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
    mImGuiCpuHandle = mSrvHeap->GetCPUDescriptorHandleForHeapStart();
    mImGuiGpuHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
    mSourceCpuHandle = mImGuiCpuHandle;
    mSourceCpuHandle.ptr += static_cast<SIZE_T>(SrvStep);
    mSourceGpuHandle = mImGuiGpuHandle;
    mSourceGpuHandle.ptr += static_cast<UINT64>(SrvStep);
    mCompressedCpuHandle = mSourceCpuHandle;
    mCompressedCpuHandle.ptr += static_cast<SIZE_T>(SrvStep);
    mCompressedGpuHandle = mSourceGpuHandle;
    mCompressedGpuHandle.ptr += static_cast<UINT64>(SrvStep);
    return true;
}

bool ViewerApplication::InitializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Io { ImGui::GetIO() };
    Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplWin32_Init(mWindowHandle)) {
        return false;
    }
    if (!ImGui_ImplDX12_Init(mDevice.Get(), FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, mSrvHeap.Get(), mImGuiCpuHandle, mImGuiGpuHandle)) {
        return false;
    }
    return true;
}

void ViewerApplication::ShutdownImGui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ViewerApplication::ShutdownD3D12() {
    if (mFenceEvent != nullptr) {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
}

bool ViewerApplication::BeginFrame() {
    if (FAILED(mCommandAllocators[mFrameIndex]->Reset())) {
        return false;
    }
    if (FAILED(mCommandList->Reset(mCommandAllocators[mFrameIndex].Get(), nullptr))) {
        return false;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    return true;
}

void ViewerApplication::RenderUi() {
    ImGui::Begin("Texture Artifact Analyzer");

    const bool HasTexture { mAnalyzer.HasTexture() };
    if (mIsProcessing) {
        ImGui::Text("처리 중...");
    }

    if (mHasLastError) {
        ImGui::Text("최근 작업이 실패했습니다.");
    }

    ImGui::Text("최근 로드 시간: %.3f 초", mLastLoadSeconds);
    ImGui::Text("최근 압축 시간: %.3f 초", mLastCompressSeconds);

    if (!mIsProcessing) {
        if (!mFormatOptions.empty()) {
            std::vector<const char*> FormatNames {};
            FormatNames.reserve(mFormatOptions.size());
            for (size_t Index { 0 }; Index < mFormatOptions.size(); ++Index) {
                FormatNames.push_back(mFormatOptions[Index].Name.c_str());
            }
            if (ImGui::Combo("Format", &mSelectedFormatIndex, FormatNames.data(), static_cast<int>(FormatNames.size()))) {
                mSettings.Format = mFormatOptions[static_cast<size_t>(mSelectedFormatIndex)].Format;
                ApplySettingsAndRefreshPreview();
            }
        }

        const char* QualityItems[] { "Fast", "Normal", "Best" };
        int QualityIndex { static_cast<int>(mSettings.CompressionQuality) };
        if (ImGui::Combo("Compression Quality", &QualityIndex, QualityItems, 3)) {
            mSettings.CompressionQuality = static_cast<CompressionQualityLevel>(QualityIndex);
            ApplySettingsAndRefreshPreview();
        }

        const char* MipItems[] { "Point", "Box", "Linear", "Fant", "Kaiser" };
        int MipIndex { 3 };
        if (mSettings.MipFilter == TEX_FILTER_POINT) {
            MipIndex = 0;
        }
        if (mSettings.MipFilter == TEX_FILTER_BOX) {
            MipIndex = 1;
        }
        if (mSettings.MipFilter == TEX_FILTER_LINEAR) {
            MipIndex = 2;
        }
        if (mSettings.MipFilter == TEX_FILTER_FANT) {
            MipIndex = 3;
        }
        if (mSettings.MipFilter == TEX_FILTER_CUBIC) {
            MipIndex = 4;
        }
        if (ImGui::Combo("Mipmap Filter", &MipIndex, MipItems, 5)) {
            if (MipIndex == 0) {
                mSettings.MipFilter = TEX_FILTER_POINT;
            }
            if (MipIndex == 1) {
                mSettings.MipFilter = TEX_FILTER_BOX;
            }
            if (MipIndex == 2) {
                mSettings.MipFilter = TEX_FILTER_LINEAR;
            }
            if (MipIndex == 3) {
                mSettings.MipFilter = TEX_FILTER_FANT;
            }
            if (MipIndex == 4) {
                mSettings.MipFilter = TEX_FILTER_CUBIC;
            }
            ApplySettingsAndRefreshPreview();
        }

        if (ImGui::Checkbox("Generate Mipmaps", &mSettings.GenerateMipmaps)) {
            ApplySettingsAndRefreshPreview();
        }
        if (ImGui::Checkbox("sRGB", &mSettings.IsSrgb)) {
            ApplySettingsAndRefreshPreview();
        }
        if (ImGui::Checkbox("Normal Map Mode", &mSettings.IsNormalMap)) {
            ApplySettingsAndRefreshPreview();
        }
        if (ImGui::Checkbox("Reconstruct Z", &mSettings.ReconstructZ)) {
            ApplySettingsAndRefreshPreview();
        }
        if (ImGui::SliderFloat("Alpha Weight", &mSettings.AlphaWeight, 0.0f, 2.0f)) {
            ApplySettingsAndRefreshPreview();
        }
    } else {
        ImGui::BeginDisabled();
        if (!mFormatOptions.empty()) {
            std::vector<const char*> FormatNames {};
            FormatNames.reserve(mFormatOptions.size());
            for (size_t Index { 0 }; Index < mFormatOptions.size(); ++Index) {
                FormatNames.push_back(mFormatOptions[Index].Name.c_str());
            }
            ImGui::Combo("Format", &mSelectedFormatIndex, FormatNames.data(), static_cast<int>(FormatNames.size()));
        }
        const char* QualityItems[] { "Fast", "Normal", "Best" };
        int QualityIndex { static_cast<int>(mSettings.CompressionQuality) };
        ImGui::Combo("Compression Quality", &QualityIndex, QualityItems, 3);
        const char* MipItems[] { "Point", "Box", "Linear", "Fant", "Kaiser" };
        int MipIndex { 3 };
        ImGui::Combo("Mipmap Filter", &MipIndex, MipItems, 5);
        ImGui::Checkbox("Generate Mipmaps", &mSettings.GenerateMipmaps);
        ImGui::Checkbox("sRGB", &mSettings.IsSrgb);
        ImGui::Checkbox("Normal Map Mode", &mSettings.IsNormalMap);
        ImGui::Checkbox("Reconstruct Z", &mSettings.ReconstructZ);
        ImGui::SliderFloat("Alpha Weight", &mSettings.AlphaWeight, 0.0f, 2.0f);
        ImGui::EndDisabled();
    }

    const char* ChannelItems[] { "RGBA", "R", "G", "B", "A", "Diff" };
    int ChannelIndex { static_cast<int>(mSettings.ChannelView) };
    if (ImGui::Combo("Channel", &ChannelIndex, ChannelItems, 6)) {
        mSettings.ChannelView = static_cast<ChannelViewMode>(ChannelIndex);
    }

    if (ImGui::Button("Save as DDS")) {
        mAnalyzer.SaveCurrentAsDds();
    }

    const TextureMemoryMetrics Metrics { mAnalyzer.GetMetrics() };
    ImGui::Text("Source: %zu bytes", Metrics.SourceBytes);
    ImGui::Text("Compressed: %zu bytes", Metrics.CompressedBytes);
    ImGui::Text("Ratio: %.3f", Metrics.CompressionRatio);
    ImGui::End();

    ImGui::Begin("Comparison");
    const ImVec2 Region { ImGui::GetContentRegionAvail() };
    const float HalfWidth { std::max(10.0f, Region.x * 0.5f - 6.0f) };
    const SyncViewportState& Viewport { mAnalyzer.GetViewportState() };

    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
        const ImVec2 MousePos { ImGui::GetMousePos() };
        mAnalyzer.HandleZoom(ImGui::GetIO().MouseWheel, XMFLOAT2 { MousePos.x, MousePos.y });
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 MousePos { ImGui::GetMousePos() };
        if (!Viewport.IsPanning) {
            mAnalyzer.BeginPan(XMFLOAT2 { MousePos.x, MousePos.y });
        }
        mAnalyzer.UpdatePan(XMFLOAT2 { MousePos.x, MousePos.y });
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        mAnalyzer.EndPan();
    }

    const ImVec2 DrawSize { HalfWidth * Viewport.Zoom, Region.y * Viewport.Zoom };
    ImGui::Text("Original");
    if (mHasSourceTexture) {
        ImGui::Image(reinterpret_cast<ImTextureID>(mSourceGpuHandle.ptr), DrawSize);
    } else {
        ImGui::Dummy(DrawSize);
    }
    ImGui::SameLine();
    ImGui::Text("Compressed");
    if (mHasCompressedTexture) {
        ImGui::Image(reinterpret_cast<ImTextureID>(mCompressedGpuHandle.ptr), DrawSize);
    } else {
        ImGui::Dummy(DrawSize);
    }
    if (!HasTexture) {
        ImGui::Text("텍스처를 드래그 앤 드롭하면 실시간 압축 결과를 표시합니다.");
    }
    ImGui::End();
}

void ViewerApplication::RenderFrame() {
    ImGui::Render();
    const D3D12_RESOURCE_BARRIER ToRenderTarget { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET } };
    mCommandList->ResourceBarrier(1, &ToRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle { mRtvHeap->GetCPUDescriptorHandleForHeapStart() };
    RtvHandle.ptr += static_cast<SIZE_T>(mFrameIndex) * static_cast<SIZE_T>(mRtvDescriptorSize);
    const float ClearColor[4] { 0.1f, 0.1f, 0.1f, 1.0f };
    mCommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
    mCommandList->ClearRenderTargetView(RtvHandle, ClearColor, 0, nullptr);

    ID3D12DescriptorHeap* Heaps[] { mSrvHeap.Get() };
    mCommandList->SetDescriptorHeaps(1, Heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    const D3D12_RESOURCE_BARRIER ToPresent { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT } };
    mCommandList->ResourceBarrier(1, &ToPresent);
}

void ViewerApplication::EndFrame() {
    mCommandList->Close();
    ID3D12CommandList* Lists[] { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, Lists);
    mSwapChain->Present(1, 0);
    MoveToNextFrame();
}

void ViewerApplication::WaitForGpu() {
    const UINT64 FenceToWait { mFenceValue };
    mCommandQueue->Signal(mFence.Get(), FenceToWait);
    mFenceValue += 1;
    if (mFence->GetCompletedValue() < FenceToWait) {
        mFence->SetEventOnCompletion(FenceToWait, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

void ViewerApplication::MoveToNextFrame() {
    const UINT64 CurrentFenceValue { mFenceValue };
    mCommandQueue->Signal(mFence.Get(), CurrentFenceValue);
    mFenceValue += 1;
    if (mFence->GetCompletedValue() < CurrentFenceValue) {
        mFence->SetEventOnCompletion(CurrentFenceValue, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void ViewerApplication::HandleDroppedFile(HDROP DropHandle) {
    std::wstring FilePath {};
    FilePath.resize(MAX_PATH);
    const UINT Length { DragQueryFileW(DropHandle, 0, FilePath.data(), static_cast<UINT>(FilePath.size())) };
    DragFinish(DropHandle);
    if (Length == 0) {
        return;
    }
    FilePath.resize(Length);
    mPendingDropPath = std::filesystem::path { FilePath };
    mHasPendingDrop = true;
}

void ViewerApplication::ProcessPendingDrop() {
    if (!mHasPendingDrop) {
        return;
    }
    mHasPendingDrop = false;
    mIsProcessing = true;
    mHasLastError = false;
    const auto StartLoadTime { std::chrono::steady_clock::now() };
    const bool IsLoaded { mAnalyzer.LoadTexture(mPendingDropPath) };
    const auto EndLoadTime { std::chrono::steady_clock::now() };
    mLastLoadSeconds = std::chrono::duration<double>(EndLoadTime - StartLoadTime).count();
    mLastCompressSeconds = mLastLoadSeconds;
    mIsProcessing = false;
    if (!IsLoaded) {
        mHasLastError = true;
        return;
    }
    RefreshSourceTexture();
    RefreshCompressedTexture();
}

bool ViewerApplication::ApplySettingsWithoutGpuRefresh() {
    if (!mAnalyzer.HasTexture()) {
        return false;
    }
    mIsProcessing = true;
    const auto StartCompressTime { std::chrono::steady_clock::now() };
    const bool IsApplied { mAnalyzer.ApplySettings(mSettings) };
    const auto EndCompressTime { std::chrono::steady_clock::now() };
    mLastCompressSeconds = std::chrono::duration<double>(EndCompressTime - StartCompressTime).count();
    mIsProcessing = false;
    return IsApplied;
}

void ViewerApplication::ApplySettingsAndRefreshPreview() {
    mHasLastError = false;
    if (ApplySettingsWithoutGpuRefresh()) {
        RefreshCompressedTexture();
        return;
    }
    if (mAnalyzer.HasTexture()) {
        mHasLastError = true;
    }
}

void ViewerApplication::RefreshSourceTexture() {
    mCommandAllocators[mFrameIndex]->Reset();
    mCommandList->Reset(mCommandAllocators[mFrameIndex].Get(), nullptr);
    mHasSourceTexture = false;
    if (mAnalyzer.UpdateSourceGpuResources(mDevice.Get(), mCommandList.Get(), mUploader, mSourceTexture, mSourceUpload)) {
        if (mSourceTexture.Get() != nullptr) {
            D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc {};
            SrvDesc.Format = mSourceTexture->GetDesc().Format;
            SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            SrvDesc.Texture2D.MostDetailedMip = 0;
            SrvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);
            SrvDesc.Texture2D.PlaneSlice = 0;
            SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            mDevice->CreateShaderResourceView(mSourceTexture.Get(), &SrvDesc, mSourceCpuHandle);
            mHasSourceTexture = true;
        }
    }
    mCommandList->Close();
    ID3D12CommandList* Lists[] { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, Lists);
    WaitForGpu();
}

void ViewerApplication::RefreshCompressedTexture() {
    mCommandAllocators[mFrameIndex]->Reset();
    mCommandList->Reset(mCommandAllocators[mFrameIndex].Get(), nullptr);
    mHasCompressedTexture = false;
    if (mAnalyzer.UpdatePreviewGpuResources(mDevice.Get(), mCommandList.Get(), mUploader, mCompressedTexture, mCompressedUpload)) {
        if (mCompressedTexture.Get() != nullptr) {
            D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc {};
            SrvDesc.Format = mCompressedTexture->GetDesc().Format;
            SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            SrvDesc.Texture2D.MostDetailedMip = 0;
            SrvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);
            SrvDesc.Texture2D.PlaneSlice = 0;
            SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            mDevice->CreateShaderResourceView(mCompressedTexture.Get(), &SrvDesc, mCompressedCpuHandle);
            mHasCompressedTexture = true;
        }
    }
    mCommandList->Close();
    ID3D12CommandList* Lists[] { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, Lists);
    WaitForGpu();
}

int APIENTRY wWinMain(_In_ HINSTANCE InstanceHandle, _In_opt_ HINSTANCE PreviousHandle, _In_ LPWSTR CommandLine, _In_ int ShowCommand) {
    (void)PreviousHandle;
    (void)CommandLine;
    ViewerApplication Application {};
    if (!Application.Initialize(InstanceHandle, ShowCommand)) {
        return -1;
    }
    return Application.Run();
}
