#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <wrl/client.h>
#include <dxgiformat.h>
#include <d3d12.h>
#include <DirectXTex.h>
#include <DirectXMath.h>

struct AnalyzerSettings {
    DXGI_FORMAT Format;
    TEX_FILTER_FLAGS MipFilter;
    bool GenerateMipmaps;
    bool IsSrgb;
    bool IsNormalMap;
    bool ReconstructZ;
    float AlphaWeight;
    uint32_t CompressionFlags;
};

struct TextureMemoryMetrics {
    size_t SourceBytes;
    size_t CompressedBytes;
    double CompressionRatio;
};

struct SyncViewportState {
    float Zoom;
    DirectX::XMFLOAT2 Pan;
    DirectX::XMFLOAT2 LastMousePos;
    bool IsPanning;
};

class TextureDocument {
public:
    TextureDocument();
    ~TextureDocument();
    TextureDocument(const TextureDocument& Other);
    TextureDocument& operator=(const TextureDocument& Other);
    TextureDocument(TextureDocument&& Other) noexcept;
    TextureDocument& operator=(TextureDocument&& Other) noexcept;

public:
    bool LoadFromFile(const std::filesystem::path& FilePath);
    const DirectX::ScratchImage& GetSourceImage() const;
    const DirectX::TexMetadata& GetMetadata() const;
    const std::filesystem::path& GetPath() const;

private:
    DirectX::ScratchImage mSourceImage;
    DirectX::TexMetadata mMetadata;
    std::filesystem::path mPath;
};

class CompressionPreviewCache {
public:
    CompressionPreviewCache();
    ~CompressionPreviewCache();
    CompressionPreviewCache(const CompressionPreviewCache& Other);
    CompressionPreviewCache& operator=(const CompressionPreviewCache& Other);
    CompressionPreviewCache(CompressionPreviewCache&& Other) noexcept;
    CompressionPreviewCache& operator=(CompressionPreviewCache&& Other) noexcept;

public:
    bool Rebuild(const TextureDocument& Document, const AnalyzerSettings& Settings);
    bool SaveAsDds(const std::filesystem::path& OutputPath) const;
    TextureMemoryMetrics BuildMetrics() const;
    const DirectX::ScratchImage& GetCompressedImage() const;
    const DirectX::TexMetadata& GetMetadata() const;

private:
    DirectX::ScratchImage mCompressedImage;
    DirectX::TexMetadata mMetadata;
};

class Dx12TextureUploader {
public:
    Dx12TextureUploader();
    ~Dx12TextureUploader();
    Dx12TextureUploader(const Dx12TextureUploader& Other);
    Dx12TextureUploader& operator=(const Dx12TextureUploader& Other);
    Dx12TextureUploader(Dx12TextureUploader&& Other) noexcept;
    Dx12TextureUploader& operator=(Dx12TextureUploader&& Other) noexcept;

public:
    bool CreateTextureAndUpload(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const DirectX::ScratchImage& Image, Microsoft::WRL::ComPtr<ID3D12Resource>& TextureOut, Microsoft::WRL::ComPtr<ID3D12Resource>& UploadOut);

private:
    static size_t ComputeSubresourceCount(const DirectX::TexMetadata& Metadata);
};

class TextureArtifactAnalyzer {
public:
    TextureArtifactAnalyzer();
    ~TextureArtifactAnalyzer();
    TextureArtifactAnalyzer(const TextureArtifactAnalyzer& Other);
    TextureArtifactAnalyzer& operator=(const TextureArtifactAnalyzer& Other);
    TextureArtifactAnalyzer(TextureArtifactAnalyzer&& Other) noexcept;
    TextureArtifactAnalyzer& operator=(TextureArtifactAnalyzer&& Other) noexcept;

public:
    bool LoadTexture(const std::filesystem::path& FilePath);
    bool ApplySettings(const AnalyzerSettings& Settings);
    bool SaveCurrentAsDds() const;
    TextureMemoryMetrics GetMetrics() const;
    const SyncViewportState& GetViewportState() const;
    void HandleZoom(float WheelStep, const DirectX::XMFLOAT2& MousePos);
    void BeginPan(const DirectX::XMFLOAT2& MousePos);
    void UpdatePan(const DirectX::XMFLOAT2& MousePos);
    void EndPan();
    bool UpdatePreviewGpuResources(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, Dx12TextureUploader& Uploader, Microsoft::WRL::ComPtr<ID3D12Resource>& RightTextureOut, Microsoft::WRL::ComPtr<ID3D12Resource>& UploadOut);

private:
    TextureDocument mDocument;
    CompressionPreviewCache mPreviewCache;
    AnalyzerSettings mCurrentSettings;
    SyncViewportState mViewport;
};

std::vector<DXGI_FORMAT> BuildCompressionCandidateFormats();
DXGI_FORMAT ResolveSrgbVariant(DXGI_FORMAT Format, bool IsSrgb);
