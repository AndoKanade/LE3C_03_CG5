#include <d3d12.h>
#include "DXCommon.h"
#include "Logger.h"
#include "StringUtility.h"


#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lPalam);

using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;
void DXCommon::Initialize(WinAPI *winApi) {

  assert(winApi);
  this->winApi_ = winApi;
  InitDevice();
  InitCommand();
  CreateSwapChain();
  CreateDepthBuffer();
  CreateDescriptorHeaps();
  InitRenderTargetView();
  InitDepthStancilView();
  InitFence();
  InitViewportRect();
  InitScissorRect();
  CreateDXCCompiler();
  InitImGui();
}

void DXCommon::InitDevice() {

  HRESULT hr;

  /// デバッグレイヤーをオン
  Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    // デバッグレイヤーを有効にする
    debugController->EnableDebugLayer();

    // GPUでもチェックするようにする
    debugController->SetEnableGPUBasedValidation(TRUE);
  }

  /// DXGIファクトリーの生成

  hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

  assert(SUCCEEDED(hr));

  /// アダプターの選定
  // 使用するアダプターの変数、最初はnullptrを入れておく
  Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

  // 良い順にアダプターを頼む
  for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       i++) {

    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};

    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr));

    // ソフトウェアアダプター出なければ採用
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
      Log(std::format("Use Adapter : {}\n",
                      ConvertString(adapterDesc.Description)));
      break;
    }
    useAdapter = nullptr;
  }

  assert(useAdapter != nullptr);

  /// D3D12デバイスの生成

  D3D_FEATURE_LEVEL featureLevels[] = {

      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};

  const char *featureLevelStrings[] = {"12.2", "12.1", "12.0"};

  // 高い順に生成できるか試していく
  for (size_t i = 0; i < _countof(featureLevels); i++) {
    hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i],
                           IID_PPV_ARGS(&device));

    if (SUCCEEDED(hr)) {
      // 生成できたのでループを抜ける
      Log(std::format("FeatureLevels : {}\n", featureLevelStrings[i]));
      break;
    }
  }

  // 生成がうまくいかなかったので起動しない
  assert(SUCCEEDED(hr));
  Log("Complete create D3D12Device!!\n");
#pragma region エラー放置しない処理
#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
  if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // やばいエラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    // 警告時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    // エラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

    // 抑制するメッセージの設定
    D3D12_MESSAGE_ID denyIds[] = {
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};

    // 抑制するレベル
    D3D12_MESSAGE_SEVERITY severrities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severrities);
    filter.DenyList.pSeverityList = severrities;

    // 指定したメッセージの表示を抑制する
    infoQueue->PushStorageFilter(&filter);
  }

#endif
#pragma endregion
}

void DXCommon::InitCommand() {

  HRESULT hr;
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));

  // コマンドキューの生成に失敗したら起動しない
  assert(SUCCEEDED(hr));

  // コマンドアロケータの生成
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(&commandAllocator));

  // コマンドリストを生成する
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator.Get(), nullptr,
                                 IID_PPV_ARGS(&commandList));
  // コマンドリストの生成に失敗したら起動しない
  assert(SUCCEEDED(hr));
}

void DXCommon::CreateSwapChain() {

  HRESULT hr;
  swapChainDesc.Width = winApi_->kCliantWidth;
  swapChainDesc.Height = winApi_->kCliantHeight;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 2;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  // コマンドキュー、ウィンドウハンドル、スワップチェインの設定

  hr = dxgiFactory->CreateSwapChainForHwnd(
      commandQueue.Get(), winApi_->GetHwnd(), &swapChainDesc, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(swapChain.GetAddressOf()));
  // スワップチェインの生成に失敗したら起動しない
  assert(SUCCEEDED(hr));
}

void DXCommon::CreateDepthBuffer() {

  D3D12_RESOURCE_DESC resourceDesc{};

  resourceDesc.Width = winApi_->kCliantWidth;
  resourceDesc.Height = winApi_->kCliantHeight;
  resourceDesc.MipLevels = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  // 利用するheapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};

  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_CLEAR_VALUE depthClearValue{};

  depthClearValue.DepthStencil.Depth = 1.0f;
  depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

  HRESULT hr = device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, &depthClearValue,
      IID_PPV_ARGS(&depthStencilResource));
  assert(SUCCEEDED(hr));
}

void DXCommon::CreateDescriptorHeaps() {

  descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  descriptorSizeRTV =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  descriptorSizeDSV =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  rtvDescriptorHeap =
      CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

  srvDescriptorHeap =
      CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 28, true);

  dsvDescriptorHeap =
      CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DXCommon::InitRenderTargetView() {

  HRESULT hr;
  const UINT kNumBackBuffers = 2;

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

  for (uint32_t i = 0; i < kNumBackBuffers; ++i) {

    hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
    assert(SUCCEEDED(hr));

    rtvHandles[i] = rtvHandle;

    device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc,
                                   rtvHandle);

    rtvHandle.ptr += descriptorSizeRTV;
  }
}

void DXCommon::InitDepthStancilView() {

  dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // Flagsの初期化も明示的に行うべき

  device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc,
                                 dsvHandle // メンバー変数を渡す
  );
}

void DXCommon::InitFence() {
  HRESULT hr;

  hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&fence));
  assert(SUCCEEDED(hr));

  fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  assert(fenceEvent != nullptr);
}

void DXCommon::InitViewportRect() {

  viewport.Width = WinAPI::kCliantWidth;
  viewport.Height = WinAPI::kCliantHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
}

void DXCommon::InitScissorRect() {
  // Scissor
  scissorRect.left = 0;
  scissorRect.right = WinAPI::kCliantWidth;
  scissorRect.top = 0;
  scissorRect.bottom = WinAPI::kCliantHeight;
}

void DXCommon::CreateDXCCompiler() {
  HRESULT hr;

  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  assert(SUCCEEDED(hr));
  hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
  assert(SUCCEEDED(hr));
}

void DXCommon::InitImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(winApi_->GetHwnd());
  ImGui_ImplDX12_Init(device.Get(), swapChainDesc.BufferCount, rtvDesc.Format,
                      srvDescriptorHeap.Get(),
                      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

ComPtr<ID3D12DescriptorHeap>
DXCommon::CreateDiscriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                               UINT numDescriptors, bool shaderVisible) {

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
  descriptorHeapDesc.Type = heapType;
  descriptorHeapDesc.NumDescriptors = numDescriptors;
  descriptorHeapDesc.Flags = shaderVisible
                                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc,
                                            IID_PPV_ARGS(&descriptorHeap));
  assert(SUCCEEDED(hr));

  return descriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE
DXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
  return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE
DXCommon::GetSRVGPUDescriptorHandle(uint32_t index) {
  return GetGPUDscriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

Microsoft::WRL::ComPtr<IDxcBlob>
DXCommon::CompileShader(const std::wstring &filePath, const wchar_t *profile) {
  Log(StringUtility::ConvertString(std::format(
      L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

  Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
  HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr,
                                  shaderSource.GetAddressOf());

  assert(SUCCEEDED(hr));

  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8;

  LPCWSTR arguments[] = {
      filePath.c_str(), // コンパイル対象のファイル名
      L"-E",
      L"main", // エントリーポイントの指定
      L"-T",
      profile, // ShaderCompileの設定
      L"-Zi",
      L"-Qembed_debug", // デバッグ用の情報を埋め込む
      L"-Od",           // 最適化を外しておく
      L"-Zpr",          // メモリレイアウトは行優先
  };

  Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
  hr = dxcCompiler->Compile(&shaderSourceBuffer, // 読み込んだファイル
                            arguments,           // コンパイルオプション
                            _countof(arguments), // コンパイルオプションの数
                            includeHandler,      // includeが含まれた数
                            IID_PPV_ARGS(&shaderResult) // コンパイル結果
  );

  assert(SUCCEEDED(hr));

  Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());

    assert(false);
  }

  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  assert(SUCCEEDED(hr));

  Log(StringUtility::ConvertString(std::format(
      L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

  return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DXCommon::CreateBufferResource(size_t sizeInBytes) {
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = sizeInBytes;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

  HRESULT hr = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));

  assert(SUCCEEDED(hr));

  return buffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DXCommon::CreateTextureResource(const DirectX::TexMetadata &metadata) {
  // meradataをもとにresourceの設定
  D3D12_RESOURCE_DESC resourceDesc = {};
  resourceDesc.Width = UINT(metadata.width);
  resourceDesc.Height = UINT(metadata.height);
  resourceDesc.MipLevels = UINT16(metadata.mipLevels);
  resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
  resourceDesc.Format = metadata.format;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

  // 利用するheapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};

  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

  // resourceを生成

  Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));

  return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> DXCommon::UploadTextureData(
    const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
    const DirectX::ScratchImage &mipImages) {
  std::vector<D3D12_SUBRESOURCE_DATA> subresources;
  DirectX::PrepareUpload(device.Get(), mipImages.GetImages(),
                         mipImages.GetImageCount(), mipImages.GetMetadata(),
                         subresources);
  uint64_t intermediateSize =
      GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
  Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
      CreateBufferResource(intermediateSize);
  UpdateSubresources(commandList.Get(), texture.Get(),
                     intermediateResource.Get(), 0, 0,
                     UINT(subresources.size()), subresources.data());

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = texture.Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
  commandList->ResourceBarrier(1, &barrier);
  return intermediateResource;
}

DirectX::ScratchImage DXCommon::LoadTexture(const std::string &filePath) {
  DirectX::ScratchImage image{};
  std::wstring filePathW = StringUtility::ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  return mipImages;
}

void DXCommon::PreDraw() {
  UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

  barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  commandList.Get()->ResourceBarrier(1, &barrier);

  commandList.Get()->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                        &dsvHandle);
  float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f}; // ここで色を変える
  commandList.Get()->ClearRenderTargetView(rtvHandles[backBufferIndex],
                                           clearColor, 0, nullptr);

  commandList.Get()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH,
                                           1.0f, 0, 0, nullptr);

  const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = {
      srvDescriptorHeap};
  commandList.Get()->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());

  commandList.Get()->RSSetViewports(1, &viewport);
  commandList.Get()->RSSetScissorRects(1, &scissorRect);
}

void DXCommon::PostDraw() {

  HRESULT hr;

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  commandList.Get()->ResourceBarrier(1, &barrier);

  hr = commandList.Get()->Close();
  assert(SUCCEEDED(hr));

  const Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = {
      commandList};
  commandQueue.Get()->ExecuteCommandLists(1, commandLists->GetAddressOf());

  swapChain->Present(1, 0);

  fenceValue++;
  commandQueue->Signal(fence.Get(), fenceValue);

  if (fence->GetCompletedValue() < fenceValue) {
    fence->SetEventOnCompletion(fenceValue, fenceEvent);

    WaitForSingleObject(fenceEvent, INFINITE);
  }
  hr = commandAllocator->Reset();
  assert(SUCCEEDED(hr));
  hr = commandList->Reset(commandAllocator.Get(), nullptr);
  assert(SUCCEEDED(hr));

  UINT bbIndex = swapChain->GetCurrentBackBufferIndex();
}
