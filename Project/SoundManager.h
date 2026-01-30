#pragma once

#include <wrl.h>
#include <xaudio2.h>
#include <string>
#include <map>
#include <cstdint>

// XAudio2ライブラリのリンク設定
#pragma comment(lib, "xaudio2.lib")

/// <summary>
/// 音声データ構造体
/// WAVファイルのフォーマット情報と波形データを保持する
/// </summary>
struct SoundData{
	WAVEFORMATEX wfex;      // 波形フォーマット
	BYTE* pBuffer;          // 波形データのバッファ
	unsigned int bufferSize;// バッファのサイズ
};

/// <summary>
/// サウンド管理クラス (シングルトン)
/// WAVファイルのロード、再生、停止、一時停止などを管理する
/// </summary>
class SoundManager{
public: // --- シングルトンインスタンス取得 ---

	static SoundManager* GetInstance();

public: // --- システム初期化・終了 ---

	// 初期化 (XAudio2エンジンの生成など)
	void Initialize();

	// 終了処理 (読み込んだ音声データの解放など)
	void Finalize();

public: // --- 音声ロード・再生制御 ---

	// WAVファイルをロード (filenameをキーとして管理)
	void LoadWave(const std::string& filename);

	// 音声を再生
	// volume: 0.0f(無音) ～ 1.0f(最大), loop: trueで無限ループ
	void PlayWave(const std::string& filename,float volume = 1.0f,bool loop = false);

	// 音声を停止 (停止後は最初から再生になる)
	void StopWave(const std::string& filename);

	// 一時停止 (再生位置を保持したまま止める)
	void PauseWave(const std::string& filename);

	// 再開 (一時停止した位置から再生する)
	void ResumeWave(const std::string& filename);

	// 再生中かどうかを確認
	bool IsPlaying(const std::string& filename);

private: // --- コンストラクタ・デストラクタ (外部からの生成禁止) ---
	SoundManager() = default;
	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

private: // --- 内部ヘルパー関数 ---

	// 音声データのメモリ解放
	void Unload(SoundData* soundData);

private: // --- メンバ変数 ---

	// XAudio2本体
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	// マスターボイス (最終的な出力先)
	IXAudio2MasteringVoice* masterVoice_ = nullptr;

	// ロード済み音声データの管理コンテナ [キー:ファイル名]
	std::map<std::string,SoundData> soundDatas_;

	// 再生中のソースボイス管理コンテナ [キー:ファイル名]
	// 停止や一時停止制御のためにインスタンスを保持しておく
	std::map<std::string,IXAudio2SourceVoice*> activeVoices_;
};