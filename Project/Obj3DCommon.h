#pragma once
#include "DXCommon.h"
#include <d3d12.h>
#include <wrl.h>

class Obj3dCommon{

public: // 外部から呼び出すもの

    // 初期化
    void Initialize(DXCommon* dxCommon);

    // 描画設定（コマンドリストへのセットなど）
    void Draw();

    // ゲッター（読み取り専用）
    // const をつけることで、取得したポインタの中身を書き換えられないことを保証できる場合もありますが、
    // DirectXのポインタはそのまま渡すのが一般的なのでこのままでOKです。
    ID3D12RootSignature* GetRootSignature() const{ return rootSignature.Get(); }
    ID3D12PipelineState* GetGraphicsPipelineState() const{ return graphicsPipelineState.Get(); }
    DXCommon* GetDxCommon() const{ return dxCommon_; }

private: // 内部でしか使わない関数
    void CreateRootSignature();
    void CreateGraphicsPipelineState();

private: // 内部データ（外部から勝手に書き換えられたくない）

    DXCommon* dxCommon_ = nullptr;

    // 最終的な成果物だけを持っておく
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};