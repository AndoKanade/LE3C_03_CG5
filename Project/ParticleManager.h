#pragma once
#include "DXCommon.h"
#include "SrvManager.h"
#include "Math.h" 
#include <list>
#include <random>
#include <map>
#include <unordered_map> // unordered_map用に明示
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <memory> // ★追加: unique_ptr用

// 既存の構造体
struct Particle{
    Transform transform;
    Vector3 velocity;
    Vector4 color;
    float lifeTime;
    float currentTime;
};

struct ParticleForGPU{
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

// パーティクルグループ構造体
class Camera;
struct ParticleGroup{
    // マテリアルデータ
    std::string textureFilePath;
    uint32_t textureSrvIndex;

    // パーティクルのリスト
    std::list<Particle> particles;

    // インスタンシング用データ (グループごとに持つ)
    uint32_t instancingSrvIndex;
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
    const uint32_t kNumMaxInstance = 1000; // グループごとの最大数
    ParticleForGPU* instancingData = nullptr; // マップ済みポインタ(解放不要)
    uint32_t numInstance = 0;
};

class ParticleManager{
public: // --- シングルトン ---
    static ParticleManager* GetInstance();

    // ★変更: staticを外す
    void Finalize();

    // コピー禁止
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

private:
    // ★削除: static ParticleManager* instance; は不要

    ParticleManager() = default;
    ~ParticleManager() = default;

public: // --- メンバ関数 ---

    // 初期化（共有リソースの生成のみを行う）
    void Initialize(DXCommon* dxCommon,SrvManager* srvManager);

    // パーティクルグループの生成
    void CreateParticleGroup(const std::string& name,const std::string& textureFilePath);

    // 更新
    void Update(Camera* camera);

    // 描画
    void Draw(const Matrix4x4& viewProjectionMatrix);

    // パーティクル発生 (エミット)
    void Emit(const std::string& name,const Vector3& position,uint32_t count);

private: // --- メンバ変数 ---

    DXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    std::mt19937 randomEngine_;

    // ★変更: std::unique_ptr で管理する
    // これにより、グループのコピーが発生せず、メモリ管理が自動化されます
    std::unordered_map<std::string,std::unique_ptr<ParticleGroup>> particleGroups_;

    // --- 共有リソース (全グループ共通) ---
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    // vertexResource_ は VertexBuffer を作るための一時的なものならメンバ変数でなくても良いですが、
    // ここではそのままにしておきます。
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Particle MakeNewParticle(const Vector3& translate);

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;

    // 頂点データの構造体
    struct VertexData{
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    void CreateGraphicsPipeline();
    void CreateModel();
};