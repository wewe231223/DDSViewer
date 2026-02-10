#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <array>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include "TextureArtifactAnalyzer.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

#pragma comment(lib, "DirectXTex.lib")


class ViewerApplication {
public:
    using ComPtrDevice = Microsoft::WRL::ComPtr<ID3D12Device>;
    using ComPtrCommandQueue = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;
    using ComPtrSwapChain = Microsoft::WRL::ComPtr<IDXGISwapChain3>;
    using ComPtrAllocator = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
    using ComPtrCommandList = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
    using ComPtrFence = Microsoft::WRL::ComPtr<ID3D12Fence>;
    using ComPtrResource = Microsoft::WRL::ComPtr<ID3D12Resource>;
    using ComPtrHeap = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;

public:
    ViewerApplication();
    ~ViewerApplication();
    ViewerApplication(const ViewerApplication& Other);
    ViewerApplication& operator=(const ViewerApplication& Other);
    ViewerApplication(ViewerApplication&& Other) noexcept;
    ViewerApplication& operator=(ViewerApplication&& Other) noexcept;

public:
    bool Initialize(HINSTANCE InstanceHandle, int ShowCommand);
    int Run();

private:
    static LRESULT CALLBACK StaticWindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);
    LRESULT WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

    bool CreateMainWindow(HINSTANCE InstanceHandle, int ShowCommand);
    bool InitializeD3D12();
    bool InitializeImGui();
    void ShutdownImGui();
    void ShutdownD3D12();

    bool BeginFrame();
    void RenderUi();
    void RenderFrame();
    void EndFrame();

    void WaitForGpu();
    void MoveToNextFrame();

    void HandleDroppedFile(HDROP DropHandle);
    void ProcessPendingDrop();
    void ApplySettingsAndRefreshPreview();
    bool ApplySettingsWithoutGpuRefresh();
    void RefreshSourceTexture();
    void RefreshCompressedTexture();

private:
    static constexpr UINT FrameCount { 2 };

    HWND mWindowHandle;
    wchar_t mWindowClassName[64];
    bool mRunning;

    ComPtrDevice mDevice;
    ComPtrCommandQueue mCommandQueue;
    ComPtrSwapChain mSwapChain;
    std::array<ComPtrAllocator, FrameCount> mCommandAllocators;
    ComPtrCommandList mCommandList;
    std::array<ComPtrResource, FrameCount> mRenderTargets;
    ComPtrHeap mRtvHeap;
    ComPtrHeap mSrvHeap;
    UINT mRtvDescriptorSize;
    UINT mFrameIndex;

    ComPtrFence mFence;
    UINT64 mFenceValue;
    HANDLE mFenceEvent;

    TextureArtifactAnalyzer mAnalyzer;
    Dx12TextureUploader mUploader;
    AnalyzerSettings mSettings;
    std::vector<FormatOption> mFormatOptions;
    int mSelectedFormatIndex;

    ComPtrResource mSourceTexture;
    ComPtrResource mSourceUpload;
    ComPtrResource mCompressedTexture;
    ComPtrResource mCompressedUpload;

    D3D12_CPU_DESCRIPTOR_HANDLE mImGuiCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mImGuiGpuHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE mSourceCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mSourceGpuHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE mCompressedCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mCompressedGpuHandle;

    bool mHasSourceTexture;
    bool mHasCompressedTexture;
    bool mHasPendingDrop;
    bool mIsProcessing;
    bool mHasLastError;
    double mLastLoadSeconds;
    double mLastCompressSeconds;
    std::filesystem::path mPendingDropPath;
};
