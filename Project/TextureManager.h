#pragma once
#include <string>
#include <vector> // vectorが抜けていたので追加
#include <unordered_map> // これも必要なら追加
#include <DirectXTex.h>
#include <wrl.h>
#include <d3d12.h>

class DXCommon;

class TextureManager{
private: // ▼ 自分だけが知っていればいい情報

    // シングルトン管理用（勝手にnewされないように）
    static TextureManager* instance;
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(TextureManager&) = delete;
    TextureManager& operator=(TextureManager&) = delete;

    // テクスチャ1枚分のデータ構造体
    // (外部で struct TextureData を直接作らないなら private でOK)
    struct TextureData{
        std::string filePath;
        DirectX::TexMetadata metadata;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
    };

    // テクスチャデータ一覧
    // (メンバ変数には「_」をつけるルールに統一すると見やすいです)
    std::vector<TextureData> textureDatas_;

    static uint32_t kSRVIndexTop;

    // DXCommonのポインタ
    DXCommon* dxCommon_ = nullptr;

    // ※さっきのコードにあった map は一旦消しました。
    //   vectorで管理するコードを書いたので、vectorだけで十分です。

public: // ▼ 外部（MainやSprite）から呼び出したい機能

    // シングルトン取得
    static TextureManager* GetInstance();

    // 初期化・終了
    void Initialize(DXCommon* dxCommon);
    void Finalize();

    // テクスチャ読み込み
    void LoadTexture(const std::string& filePath);

    // ★追加: テクスチャのGPUハンドルを取得する関数
    // (これがないと、Spriteクラスが「どの画像を描画するか」を設定できません)
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

    // ★追加: テクスチャのメタデータ（サイズなど）を取得する関数
    // (スプライトのサイズを画像のサイズに合わせたい時などに使います)
    const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

    uint32_t GetTextureIndexbyFilePath(const std::string& filePath);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

	const DirectX::TexMetadata& GetMetaData(uint32_t textureIndex);

};