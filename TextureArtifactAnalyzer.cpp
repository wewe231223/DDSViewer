#include "TextureArtifactAnalyzer.h"

#include <algorithm>
#include <array>
#include <cstring>


using namespace Microsoft::WRL;



TextureDocument::TextureDocument() :
    mSourceImage {},
    mMetadata {},
    mPath {} {
}

TextureDocument::~TextureDocument() {
}

TextureDocument::TextureDocument(const TextureDocument& Other) :
    mSourceImage {},
    mMetadata { Other.mMetadata },
    mPath { Other.mPath } {
    if (Other.mSourceImage.GetPixels() != nullptr) {
        const HRESULT Hr { mSourceImage.InitializeFromImage(*Other.mSourceImage.GetImage(0, 0, 0), false) };
        if (FAILED(Hr)) {
            mSourceImage.Release();
        }
    }
}

TextureDocument& TextureDocument::operator=(const TextureDocument& Other) {
    if (this != &Other) {
        mSourceImage.Release();
        mMetadata = Other.mMetadata;
        mPath = Other.mPath;
        if (Other.mSourceImage.GetPixels() != nullptr) {
            const HRESULT Hr { mSourceImage.InitializeFromImage(*Other.mSourceImage.GetImage(0, 0, 0), false) };
            if (FAILED(Hr)) {
                mSourceImage.Release();
            }
        }
    }
    return *this;
}

TextureDocument::TextureDocument(TextureDocument&& Other) noexcept :
    mSourceImage { std::move(Other.mSourceImage) },
    mMetadata { Other.mMetadata },
    mPath { std::move(Other.mPath) } {
}

TextureDocument& TextureDocument::operator=(TextureDocument&& Other) noexcept {
    if (this != &Other) {
        mSourceImage = std::move(Other.mSourceImage);
        mMetadata = Other.mMetadata;
        mPath = std::move(Other.mPath);
    }
    return *this;
}

bool TextureDocument::LoadFromFile(const std::filesystem::path& FilePath) {
    mPath = FilePath;
    mSourceImage.Release();
    mMetadata = {};
    const HRESULT Hr { LoadFromWICFile(FilePath.c_str(), WIC_FLAGS_FORCE_RGB, &mMetadata, mSourceImage) };
    return SUCCEEDED(Hr);
}

const ScratchImage& TextureDocument::GetSourceImage() const {
    return mSourceImage;
}

const TexMetadata& TextureDocument::GetMetadata() const {
    return mMetadata;
}

const std::filesystem::path& TextureDocument::GetPath() const {
    return mPath;
}

CompressionPreviewCache::CompressionPreviewCache() :
    mCompressedImage {} {
}

CompressionPreviewCache::~CompressionPreviewCache() {
}

CompressionPreviewCache::CompressionPreviewCache(const CompressionPreviewCache& Other) :
    mCompressedImage {} {
    if (Other.mCompressedImage.GetPixels() != nullptr) {
        const HRESULT Hr { mCompressedImage.InitializeFromImage(*Other.mCompressedImage.GetImage(0, 0, 0), false) };
        if (FAILED(Hr)) {
            mCompressedImage.Release();
        }
    }
}

CompressionPreviewCache& CompressionPreviewCache::operator=(const CompressionPreviewCache& Other) {
    if (this != &Other) {
        mCompressedImage.Release();
        if (Other.mCompressedImage.GetPixels() != nullptr) {
            const HRESULT Hr { mCompressedImage.InitializeFromImage(*Other.mCompressedImage.GetImage(0, 0, 0), false) };
            if (FAILED(Hr)) {
                mCompressedImage.Release();
            }
        }
    }
    return *this;
}

CompressionPreviewCache::CompressionPreviewCache(CompressionPreviewCache&& Other) noexcept :
    mCompressedImage { std::move(Other.mCompressedImage) } {
}

CompressionPreviewCache& CompressionPreviewCache::operator=(CompressionPreviewCache&& Other) noexcept {
    if (this != &Other) {
        mCompressedImage = std::move(Other.mCompressedImage);
    }
    return *this;
}

bool CompressionPreviewCache::Rebuild(const TextureDocument& Document, const AnalyzerSettings& Settings) {
    if (Document.GetSourceImage().GetPixels() == nullptr) {
        return false;
    }

    ScratchImage WorkingImage {};
    const TexMetadata SourceMetadata { Document.GetMetadata() };
    if (Settings.GenerateMipmaps) {
        const HRESULT MipHr { GenerateMipMaps(Document.GetSourceImage().GetImages(), Document.GetSourceImage().GetImageCount(), SourceMetadata, Settings.MipFilter, 0, WorkingImage) };
        if (FAILED(MipHr)) {
            return false;
        }
    } else {
        const HRESULT CloneHr { WorkingImage.InitializeFromImage(*Document.GetSourceImage().GetImage(0, 0, 0), false) };
        if (FAILED(CloneHr)) {
            return false;
        }
    }

    const DXGI_FORMAT TargetFormat { ResolveSrgbVariant(Settings.Format, Settings.IsSrgb) };
    ScratchImage Compressed {};
    const TEX_COMPRESS_FLAGS Flags { BuildCompressFlags(Settings) };
    const HRESULT CompressHr { Compress(WorkingImage.GetImages(), WorkingImage.GetImageCount(), WorkingImage.GetMetadata(), TargetFormat, Flags, Settings.AlphaWeight, Compressed) };
    if (FAILED(CompressHr)) {
        return false;
    }

    mCompressedImage = std::move(Compressed);
    return true;
}

bool CompressionPreviewCache::SaveAsDds(const std::filesystem::path& OutputPath) const {
    if (mCompressedImage.GetPixels() == nullptr) {
        return false;
    }
    const HRESULT Hr { SaveToDDSFile(mCompressedImage.GetImages(), mCompressedImage.GetImageCount(), mCompressedImage.GetMetadata(), DDS_FLAGS_NONE, OutputPath.c_str()) };
    return SUCCEEDED(Hr);
}

TextureMemoryMetrics CompressionPreviewCache::BuildMetrics(const TexMetadata& SourceMetadata) const {
    TextureMemoryMetrics Metrics { 0, 0, 0.0 };
    Metrics.SourceBytes = static_cast<size_t>(SourceMetadata.width) * static_cast<size_t>(SourceMetadata.height) * 4;
    if (mCompressedImage.GetPixels() == nullptr) {
        return Metrics;
    }
    const Image* FirstImage { mCompressedImage.GetImage(0, 0, 0) };
    if (FirstImage == nullptr) {
        return Metrics;
    }
    Metrics.CompressedBytes = FirstImage->slicePitch;
    Metrics.CompressionRatio = Metrics.CompressedBytes == 0 ? 0.0 : static_cast<double>(Metrics.SourceBytes) / static_cast<double>(Metrics.CompressedBytes);
    return Metrics;
}

const ScratchImage& CompressionPreviewCache::GetCompressedImage() const {
    return mCompressedImage;
}

Dx12TextureUploader::Dx12TextureUploader() {
}

Dx12TextureUploader::~Dx12TextureUploader() {
}

Dx12TextureUploader::Dx12TextureUploader(const Dx12TextureUploader& Other) {
    (void)Other;
}

Dx12TextureUploader& Dx12TextureUploader::operator=(const Dx12TextureUploader& Other) {
    (void)Other;
    return *this;
}

Dx12TextureUploader::Dx12TextureUploader(Dx12TextureUploader&& Other) noexcept {
    (void)Other;
}

Dx12TextureUploader& Dx12TextureUploader::operator=(Dx12TextureUploader&& Other) noexcept {
    (void)Other;
    return *this;
}

size_t Dx12TextureUploader::ComputeSubresourceCount(const TexMetadata& Metadata) {
    return static_cast<size_t>(Metadata.mipLevels) * static_cast<size_t>(Metadata.arraySize);
}

bool Dx12TextureUploader::CreateTextureAndUpload(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const ScratchImage& Image, ComPtr<ID3D12Resource>& TextureOut, ComPtr<ID3D12Resource>& UploadOut) {
    if (Device == nullptr || CommandList == nullptr || Image.GetPixels() == nullptr) {
        return false;
    }

    const TexMetadata Metadata { Image.GetMetadata() };
    const D3D12_RESOURCE_DESC TextureDesc { D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, Metadata.width, static_cast<UINT>(Metadata.height), static_cast<UINT16>(Metadata.arraySize), static_cast<UINT16>(Metadata.mipLevels), Metadata.format, { 1, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE };
    const D3D12_HEAP_PROPERTIES DefaultHeap { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    const HRESULT TextureHr { Device->CreateCommittedResource(&DefaultHeap, D3D12_HEAP_FLAG_NONE, &TextureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(TextureOut.ReleaseAndGetAddressOf())) };
    if (FAILED(TextureHr)) {
        return false;
    }

    const UINT SubresourceCount { static_cast<UINT>(ComputeSubresourceCount(Metadata)) };
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints(SubresourceCount);
    std::vector<UINT> NumRows(SubresourceCount);
    std::vector<UINT64> RowSizeInBytes(SubresourceCount);
    UINT64 UploadBytes { 0 };
    Device->GetCopyableFootprints(&TextureDesc, 0, SubresourceCount, 0, Footprints.data(), NumRows.data(), RowSizeInBytes.data(), &UploadBytes);

    const D3D12_RESOURCE_DESC UploadDesc { D3D12_RESOURCE_DIMENSION_BUFFER, 0, UploadBytes, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    const D3D12_HEAP_PROPERTIES UploadHeap { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    const HRESULT UploadHr { Device->CreateCommittedResource(&UploadHeap, D3D12_HEAP_FLAG_NONE, &UploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(UploadOut.ReleaseAndGetAddressOf())) };
    if (FAILED(UploadHr)) {
        return false;
    }

    uint8_t* Mapped { nullptr };
    const HRESULT MapHr { UploadOut->Map(0, nullptr, reinterpret_cast<void**>(&Mapped)) };
    if (FAILED(MapHr) || Mapped == nullptr) {
        return false;
    }
    
    const DirectX::Image* Images { Image.GetImages() };
    for (UINT Index { 0 }; Index < SubresourceCount; ++Index) {
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Footprint { Footprints[Index] };
        uint8_t* DestBase { Mapped + Footprint.Offset };
        const DirectX::Image& Src { Images[Index] };
        for (UINT Row { 0 }; Row < NumRows[Index]; ++Row) {
            uint8_t* Dest { DestBase + static_cast<size_t>(Row) * Footprint.Footprint.RowPitch };
            const uint8_t* Source { Src.pixels + static_cast<size_t>(Row) * Src.rowPitch };
            memcpy(Dest, Source, static_cast<size_t>(RowSizeInBytes[Index]));
        }
    }
    UploadOut->Unmap(0, nullptr);

    for (UINT Index { 0 }; Index < SubresourceCount; ++Index) {
        D3D12_TEXTURE_COPY_LOCATION DstLocation {};
        DstLocation.pResource = TextureOut.Get();
        DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DstLocation.SubresourceIndex = Index;

        D3D12_TEXTURE_COPY_LOCATION SrcLocation {};
        SrcLocation.pResource = UploadOut.Get();
        SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SrcLocation.PlacedFootprint = Footprints[Index];

        CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
    }

    const D3D12_RESOURCE_BARRIER Barrier { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { TextureOut.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE } };
    CommandList->ResourceBarrier(1, &Barrier);
    return true;
}

TextureArtifactAnalyzer::TextureArtifactAnalyzer() :
    mDocument {},
    mPreviewCache {},
    mCurrentSettings { DXGI_FORMAT_BC7_UNORM, TEX_FILTER_DEFAULT, true, false, false, false, CompressionQualityLevel::Normal, ChannelViewMode::Rgba, 1.0f },
    mViewport { 1.0f, XMFLOAT2 { 0.0f, 0.0f }, XMFLOAT2 { 0.0f, 0.0f }, false } {
}

TextureArtifactAnalyzer::~TextureArtifactAnalyzer() {
}

TextureArtifactAnalyzer::TextureArtifactAnalyzer(const TextureArtifactAnalyzer& Other) :
    mDocument { Other.mDocument },
    mPreviewCache { Other.mPreviewCache },
    mCurrentSettings { Other.mCurrentSettings },
    mViewport { Other.mViewport } {
}

TextureArtifactAnalyzer& TextureArtifactAnalyzer::operator=(const TextureArtifactAnalyzer& Other) {
    if (this != &Other) {
        mDocument = Other.mDocument;
        mPreviewCache = Other.mPreviewCache;
        mCurrentSettings = Other.mCurrentSettings;
        mViewport = Other.mViewport;
    }
    return *this;
}

TextureArtifactAnalyzer::TextureArtifactAnalyzer(TextureArtifactAnalyzer&& Other) noexcept :
    mDocument { std::move(Other.mDocument) },
    mPreviewCache { std::move(Other.mPreviewCache) },
    mCurrentSettings { Other.mCurrentSettings },
    mViewport { Other.mViewport } {
}

TextureArtifactAnalyzer& TextureArtifactAnalyzer::operator=(TextureArtifactAnalyzer&& Other) noexcept {
    if (this != &Other) {
        mDocument = std::move(Other.mDocument);
        mPreviewCache = std::move(Other.mPreviewCache);
        mCurrentSettings = Other.mCurrentSettings;
        mViewport = Other.mViewport;
    }
    return *this;
}

bool TextureArtifactAnalyzer::LoadTexture(const std::filesystem::path& FilePath) {
    const bool Loaded { mDocument.LoadFromFile(FilePath) };
    if (!Loaded) {
        return false;
    }
    return mPreviewCache.Rebuild(mDocument, mCurrentSettings);
}

bool TextureArtifactAnalyzer::ApplySettings(const AnalyzerSettings& Settings) {
    mCurrentSettings = Settings;
    return mPreviewCache.Rebuild(mDocument, mCurrentSettings);
}

bool TextureArtifactAnalyzer::SaveCurrentAsDds() const {
    const std::filesystem::path SourcePath { mDocument.GetPath() };
    if (SourcePath.empty()) {
        return false;
    }
    std::filesystem::path OutputPath { SourcePath };
    OutputPath.replace_extension(L".dds");
    return mPreviewCache.SaveAsDds(OutputPath);
}

TextureMemoryMetrics TextureArtifactAnalyzer::GetMetrics() const {
    return mPreviewCache.BuildMetrics(mDocument.GetMetadata());
}

const SyncViewportState& TextureArtifactAnalyzer::GetViewportState() const {
    return mViewport;
}

const ScratchImage& TextureArtifactAnalyzer::GetSourceImage() const {
    return mDocument.GetSourceImage();
}

const ScratchImage& TextureArtifactAnalyzer::GetCompressedImage() const {
    return mPreviewCache.GetCompressedImage();
}

void TextureArtifactAnalyzer::HandleZoom(float WheelStep, const XMFLOAT2& MousePos) {
    const float PrevZoom { mViewport.Zoom };
    const float NextZoom { std::clamp(PrevZoom + WheelStep * 0.1f, 1.0f, 64.0f) };
    const float ZoomScale { NextZoom / PrevZoom };
    mViewport.Pan.x = (mViewport.Pan.x - MousePos.x) * ZoomScale + MousePos.x;
    mViewport.Pan.y = (mViewport.Pan.y - MousePos.y) * ZoomScale + MousePos.y;
    mViewport.Zoom = NextZoom;
}

void TextureArtifactAnalyzer::BeginPan(const XMFLOAT2& MousePos) {
    mViewport.IsPanning = true;
    mViewport.LastMousePos = MousePos;
}

void TextureArtifactAnalyzer::UpdatePan(const XMFLOAT2& MousePos) {
    if (!mViewport.IsPanning) {
        return;
    }
    const float DeltaX { MousePos.x - mViewport.LastMousePos.x };
    const float DeltaY { MousePos.y - mViewport.LastMousePos.y };
    mViewport.Pan.x += DeltaX;
    mViewport.Pan.y += DeltaY;
    mViewport.LastMousePos = MousePos;
}

void TextureArtifactAnalyzer::EndPan() {
    mViewport.IsPanning = false;
}

bool TextureArtifactAnalyzer::UpdateSourceGpuResources(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, Dx12TextureUploader& Uploader, ComPtr<ID3D12Resource>& LeftTextureOut, ComPtr<ID3D12Resource>& UploadOut) {
    return Uploader.CreateTextureAndUpload(Device, CommandList, mDocument.GetSourceImage(), LeftTextureOut, UploadOut);
}

bool TextureArtifactAnalyzer::UpdatePreviewGpuResources(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, Dx12TextureUploader& Uploader, ComPtr<ID3D12Resource>& RightTextureOut, ComPtr<ID3D12Resource>& UploadOut) {
    return Uploader.CreateTextureAndUpload(Device, CommandList, mPreviewCache.GetCompressedImage(), RightTextureOut, UploadOut);
}

std::vector<FormatOption> BuildCompressionCandidateFormats() {
    std::vector<FormatOption> Formats {
        { DXGI_FORMAT_R8_UNORM, "R8_UNORM" },
        { DXGI_FORMAT_R8_SNORM, "R8_SNORM" },
        { DXGI_FORMAT_R8G8_UNORM, "R8G8_UNORM" },
        { DXGI_FORMAT_R8G8_SNORM, "R8G8_SNORM" },
        { DXGI_FORMAT_R8G8B8A8_UNORM, "R8G8B8A8_UNORM" },
        { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "R8G8B8A8_UNORM_SRGB" },
        { DXGI_FORMAT_B8G8R8A8_UNORM, "B8G8R8A8_UNORM" },
        { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, "B8G8R8A8_UNORM_SRGB" },
        { DXGI_FORMAT_R10G10B10A2_UNORM, "R10G10B10A2_UNORM" },
        { DXGI_FORMAT_R11G11B10_FLOAT, "R11G11B10_FLOAT" },
        { DXGI_FORMAT_R16_FLOAT, "R16_FLOAT" },
        { DXGI_FORMAT_R16G16_FLOAT, "R16G16_FLOAT" },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, "R16G16B16A16_FLOAT" },
        { DXGI_FORMAT_R32_FLOAT, "R32_FLOAT" },
        { DXGI_FORMAT_R32G32_FLOAT, "R32G32_FLOAT" },
        { DXGI_FORMAT_R32G32B32A32_FLOAT, "R32G32B32A32_FLOAT" },
        { DXGI_FORMAT_BC1_UNORM, "BC1_UNORM" },
        { DXGI_FORMAT_BC1_UNORM_SRGB, "BC1_UNORM_SRGB" },
        { DXGI_FORMAT_BC2_UNORM, "BC2_UNORM" },
        { DXGI_FORMAT_BC2_UNORM_SRGB, "BC2_UNORM_SRGB" },
        { DXGI_FORMAT_BC3_UNORM, "BC3_UNORM" },
        { DXGI_FORMAT_BC3_UNORM_SRGB, "BC3_UNORM_SRGB" },
        { DXGI_FORMAT_BC4_UNORM, "BC4_UNORM" },
        { DXGI_FORMAT_BC4_SNORM, "BC4_SNORM" },
        { DXGI_FORMAT_BC5_UNORM, "BC5_UNORM" },
        { DXGI_FORMAT_BC5_SNORM, "BC5_SNORM" },
        { DXGI_FORMAT_BC6H_UF16, "BC6H_UF16" },
        { DXGI_FORMAT_BC6H_SF16, "BC6H_SF16" },
        { DXGI_FORMAT_BC7_UNORM, "BC7_UNORM" },
        { DXGI_FORMAT_BC7_UNORM_SRGB, "BC7_UNORM_SRGB" }
    };
    return Formats;
}

DXGI_FORMAT ResolveSrgbVariant(DXGI_FORMAT Format, bool IsSrgb) {
    if (!IsSrgb) {
        return Format;
    }
    if (Format == DXGI_FORMAT_BC1_UNORM) {
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    }
    if (Format == DXGI_FORMAT_BC2_UNORM) {
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    }
    if (Format == DXGI_FORMAT_BC3_UNORM) {
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    }
    if (Format == DXGI_FORMAT_BC7_UNORM) {
        return DXGI_FORMAT_BC7_UNORM_SRGB;
    }
    if (Format == DXGI_FORMAT_R8G8B8A8_UNORM) {
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }
    if (Format == DXGI_FORMAT_B8G8R8A8_UNORM) {
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    }
    return Format;
}

TEX_COMPRESS_FLAGS BuildCompressFlags(const AnalyzerSettings& Settings) {
    TEX_COMPRESS_FLAGS Flags { TEX_COMPRESS_DEFAULT };
    if (Settings.CompressionQuality == CompressionQualityLevel::Fast) {
        Flags = static_cast<TEX_COMPRESS_FLAGS>(Flags | TEX_COMPRESS_BC7_QUICK);
    }
    if (Settings.CompressionQuality == CompressionQualityLevel::Best) {
        Flags = static_cast<TEX_COMPRESS_FLAGS>(Flags | TEX_COMPRESS_BC7_USE_3SUBSETS);
    }
    if (Settings.IsNormalMap) {
        Flags = static_cast<TEX_COMPRESS_FLAGS>(Flags | TEX_COMPRESS_UNIFORM);
    }
    return Flags;
}
