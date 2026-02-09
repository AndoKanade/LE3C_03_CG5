#include "TitleScene.h"
#include "SceneManager.h" 
#include "ModelManager.h"
#include "Input.h"
#include "Sprite.h"       // unique_ptrの削除に必須
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "imgui.h"

// ★コンストラクタ (LNK2001エラー対策)
// ヘッダーで宣言したコンストラクタの実体です。
TitleScene::TitleScene() = default;

// ★デストラクタ (C2027エラー対策)
// ここで unique_ptr<Sprite> などが自動的に削除されます。
// Sprite.h が見えているこの場所で定義する必要があります。
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
	// ★new ではなく std::make_unique を使う
	sprite_ = std::make_unique<Sprite>();

	// 初期化と設定
	sprite_->Initialize(spriteCommon_,"resource/uvChecker.png");
	sprite_->SetPosition({0.0f, 0.0f});       // 左上に配置
	sprite_->SetSize({300.0f, 300.0f});       // 見える大きさに設定
	sprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // 真っ白（不透明）

	// --- 3Dオブジェクト生成 ---
	// ★ここも make_unique
	titleObject_ = std::make_unique<Obj3D>();

	titleObject_->Initialize(object3dCommon_);
	titleObject_->SetModel("fence.obj");

	// 座標とサイズの設定
	titleObject_->SetTranslate({0.0f, 0.0f, 0.0f});
	titleObject_->SetScale({0.5f, 0.5f, 0.5f});
}

// 終了処理
void TitleScene::Finalize(){
	// ★中身は空っぽでOK！
	// unique_ptr が自動でメモリ解放してくれるので、deleteを書く必要はありません。
	// ただし、BaseSceneで仮想関数になっているため、関数自体は消さずに残しておきます。
}

// 更新
void TitleScene::Update(){

	// --- ImGuiの処理 ---
	ImGui::Begin("Sprite Settings");

	// 色と透明度をまとめて変更
	ImGui::ColorEdit4("Color & Alpha",&spriteColor_.x);

	ImGui::End();

	// オブジェクトの更新
	if(titleObject_){
		titleObject_->Update();
	}

	// スプライトの更新
	if(sprite_){
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