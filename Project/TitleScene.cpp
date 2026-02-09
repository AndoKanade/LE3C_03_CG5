#include "TitleScene.h"
#include "SceneManager.h" 
#include "ModelManager.h"
#include "Input.h"
#include "Sprite.h"       // unique_ptrの削除に必須
#include "SpriteCommon.h"
#include "TextureManager.h"

// ★追加: ImGuiを使わない設定のときはヘッダーも読まないようにする
#ifdef USE_IMGUI
#include "imguiManager.h"
#endif

// ★コンストラクタ (LNK2001エラー対策)
TitleScene::TitleScene() = default;

// ★デストラクタ (C2027エラー対策)
TitleScene::~TitleScene(){}

// 初期化
void TitleScene::Initialize(Obj3dCommon* object3dCommon,Input* input,SpriteCommon* spriteCommon){
	object3dCommon_ = object3dCommon;
	input_ = input;
	spriteCommon_ = spriteCommon;

	// --- リソースのロード ---
	ModelManager::GetInstance()->LoadModel("fence.obj");
	TextureManager::GetInstance()->LoadTexture("resource/uvChecker.png");

	// --- スプライト生成 ---
	sprite_ = std::make_unique<Sprite>();

	// 初期化と設定
	sprite_->Initialize(spriteCommon_,"resource/uvChecker.png");
	sprite_->SetPosition({0.0f, 0.0f});       // 左上に配置
	sprite_->SetSize({300.0f, 300.0f});       // 見える大きさに設定
	sprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // 真っ白（不透明）

	// --- 3Dオブジェクト生成 ---
	titleObject_ = std::make_unique<Obj3D>();

	titleObject_->Initialize(object3dCommon_);
	titleObject_->SetModel("fence.obj");

	// 座標とサイズの設定
	titleObject_->SetTranslate({0.0f, 0.0f, 0.0f});
	titleObject_->SetScale({0.5f, 0.5f, 0.5f});
}

// 終了処理
void TitleScene::Finalize(){
	// unique_ptr が自動でメモリ解放してくれるので空でOK
}

// 更新
void TitleScene::Update(){

	// --- ImGuiの処理 ---
	// ★追加: マクロが定義されているときだけ実行する
#ifdef USE_IMGUI
	ImGui::Begin("Sprite Settings");

	// 色と透明度をまとめて変更
	ImGui::ColorEdit4("Color & Alpha",&spriteColor_.x);

	ImGui::End();
#endif

	// オブジェクトの更新
	if(titleObject_){
		titleObject_->Update();
	}

	// スプライトの更新
	if(sprite_){
		// ImGuiが無効な場合、spriteColor_ は初期値のままになります
		sprite_->SetColor(spriteColor_);
		sprite_->Update();
	}

	// シーン遷移処理 (スペースキー)
	if(input_->TriggerKey(DIK_SPACE)){
		SceneManager::GetInstance()->ChangeScene("GAME");
	}
}

// 描画
void TitleScene::Draw(){
	if(titleObject_){
		titleObject_->Draw();
	}

	if(spriteCommon_ && sprite_){
		spriteCommon_->Draw(); // 共通設定 (PreDraw)
		sprite_->Draw();       // スプライト描画
	}
}