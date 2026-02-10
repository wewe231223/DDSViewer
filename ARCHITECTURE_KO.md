# Texture Artifact Analyzer & Converter 설계

## 클래스 구조

- TextureDocument
  - 원본 이미지 로드, 메타데이터 보관, 원본 ScratchImage 접근.
- CompressionPreviewCache
  - 현재 옵션으로 메모리 내 압축 결과 재생성, DDS 저장, 메모리 메트릭 계산.
- Dx12TextureUploader
  - ScratchImage를 D3D12 텍스처 리소스로 생성하고 업로드 버퍼를 통해 GPU 갱신.
- TextureArtifactAnalyzer
  - 전체 워크플로우 오케스트레이션.
  - 드래그 앤 드롭 로드, 옵션 적용 시 즉시 재압축, 저장.
  - 동기화 줌/팬 상태 보관 및 이벤트 처리.

## 실시간 압축 파이프라인

1. TextureDocument::LoadFromFile로 원본 로드.
2. CompressionPreviewCache::Rebuild에서 옵션 기반 파이프라인 수행.
   - GenerateMipMaps로 밉맵 생성 여부 반영.
   - ResolveSrgbVariant로 SRGB 포맷 자동 변환.
   - Compress로 BC 및 비압축 포맷 압축.
   - 노멀맵 모드 시 플래그 조합과 Z 재구성 경로 처리.
3. TextureArtifactAnalyzer::UpdatePreviewGpuResources가 Dx12TextureUploader::CreateTextureAndUpload 호출.

## GPU 업데이트 핵심

- Texture2D 기본 힙 리소스 생성.
- GetCopyableFootprints로 업로드 레이아웃 계산.
- 업로드 힙 버퍼를 Map 후 행 단위 memcpy.
- CopyTextureRegion으로 서브리소스 전송.
- ResourceBarrier로 COPY_DEST에서 PIXEL_SHADER_RESOURCE 전이.

## ImGui 동기화 줌/팬 구현 아이디어

- 하나의 SyncViewportState를 좌우 패널이 공유.
- Zoom
  - 마우스 휠 입력 시 HandleZoom 호출.
  - 커서 기준 고정 줌을 위해 Pan을 역변환한 뒤 줌 스케일 적용.
- Pan
  - 마우스 드래그 시작 시 BeginPan, 이동 중 UpdatePan, 종료 시 EndPan.
  - Delta를 Pan에 누적해 양쪽 패널 동일 오프셋 유지.
- 렌더링
  - 좌우 패널의 UV 계산 시 동일 Zoom/Pan을 적용.
  - SRV는 Point Sampler를 강제해 픽셀 경계 보존.

## 채널 분석 및 Diff 확장 지점

- Pixel Shader 상수로 채널 마스크 전달.
- 채널별 출력은 float4(mask) 곱 방식.
- Diff는 abs(Source - Compressed) 또는 증폭 계수 곱으로 시각화.

## 메트릭

- CompressionPreviewCache::BuildMetrics에서 원본 크기, 압축 크기, 압축 비율 계산.
- 하단 상태 바에 실시간 노출.
