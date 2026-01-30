#include "SoundManager.h"
#include <fstream>
#include <cassert>

// ==========================================================================
// 内部用構造体定義 (WAVファイル解析用)
// ==========================================================================
struct ChunkHeader{
    char id[4];     // チャンクID ("RIFF", "fmt ", "data" 等)
    int32_t size;   // チャンクサイズ
};

struct RiffHeader{
    ChunkHeader chunk;
    char type[4];   // "WAVE"
};

struct FormatChunk{
    ChunkHeader chunk;
    WAVEFORMATEX fmt;
};

// ==========================================================================
// シングルトンインスタンス取得
// ==========================================================================
SoundManager* SoundManager::GetInstance(){
    static SoundManager instance;
    return &instance;
}

// ==========================================================================
// 初期化
// ==========================================================================
void SoundManager::Initialize(){
    HRESULT result;

    // XAudio2エンジンの作成
    result = XAudio2Create(&xAudio2_,0,XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイスの作成
    result = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(result));
}

// ==========================================================================
// 終了処理
// ==========================================================================
void SoundManager::Finalize(){
    // 1. 再生中のボイスをすべて破棄
    // (これを先にやらないと、バッファ削除後にアクセスしてクラッシュする恐れがある)
    for(auto& pair : activeVoices_){
        if(pair.second){
            pair.second->DestroyVoice();
        }
    }
    activeVoices_.clear();

    // 2. 音声データのメモリ解放
    for(auto& pair : soundDatas_){
        Unload(&pair.second);
    }
    soundDatas_.clear();

    // 3. XAudio2の解放
    xAudio2_.Reset();
}

// ==========================================================================
// WAVファイルロード
// ==========================================================================
void SoundManager::LoadWave(const std::string& filename){
    // 既にロード済みなら何もしない
    if(soundDatas_.find(filename) != soundDatas_.end()){
        return;
    }

    // ファイルオープン
    std::ifstream file;
    file.open(filename,std::ios_base::binary);
    assert(file.is_open());

    // --- RIFFヘッダーの読み込み ---
    RiffHeader riff;
    file.read((char*)&riff,sizeof(riff));
    // ファイル形式チェック
    if(strncmp(riff.chunk.id,"RIFF",4) != 0) assert(0);
    if(strncmp(riff.type,"WAVE",4) != 0) assert(0);

    // --- fmtチャンクの読み込み ---
    FormatChunk format = {};
    file.read((char*)&format,sizeof(ChunkHeader));
    if(strncmp(format.chunk.id,"fmt ",4) != 0) assert(0);

    // チャンクサイズが構造体を超えていないかチェック
    assert(format.chunk.size <= sizeof(format.fmt));
    file.read((char*)&format.fmt,format.chunk.size);

    // --- dataチャンクの読み込み ---
    ChunkHeader data;
    file.read((char*)&data,sizeof(data));

    // JUNKチャンクがあればスキップする処理
    if(strncmp(data.id,"JUNK",4) == 0){
        file.seekg(data.size,std::ios_base::cur); // 読み飛ばす
        file.read((char*)&data,sizeof(data));     // 次のチャンクヘッダを読む
    }

    // "data" チャンクであることを確認
    if(strncmp(data.id,"data",4) != 0) assert(0);

    // 波形データの読み込み
    char* pBuffer = new char[data.size];
    file.read(pBuffer,data.size);
    file.close();

    // SoundDataを作成してマップに保存
    SoundData soundData = {};
    soundData.wfex = format.fmt;
    soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
    soundData.bufferSize = data.size;

    soundDatas_[filename] = soundData;
}

// ==========================================================================
// 音声データ解放 (内部用)
// ==========================================================================
void SoundManager::Unload(SoundData* soundData){
    // バッファのメモリを解放
    delete[] soundData->pBuffer;

    soundData->pBuffer = nullptr;
    soundData->bufferSize = 0;
    soundData->wfex = {};
}

// ==========================================================================
// 音声再生
// ==========================================================================
void SoundManager::PlayWave(const std::string& filename,float volume,bool loop){
    // ロード済みデータ検索
    auto it = soundDatas_.find(filename);
    assert(it != soundDatas_.end()); // ロードされていないファイルは再生できない

    // ★ 同一名のファイルが再生中の場合、停止して再利用する (BGM等で音が重なるのを防ぐ)
    if(activeVoices_.find(filename) != activeVoices_.end()){
        StopWave(filename);
    }

    SoundData& soundData = it->second;
    HRESULT result;

    // ソースボイスの生成
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    result = xAudio2_->CreateSourceVoice(&pSourceVoice,&soundData.wfex);
    assert(SUCCEEDED(result));

    // 音量の設定
    pSourceVoice->SetVolume(volume);

    // バッファの設定
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    // ループ設定
    if(loop){
        buf.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    // 再生バッファの登録と開始
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(result));

    result = pSourceVoice->Start();
    assert(SUCCEEDED(result));

    // 再生中リストに登録
    activeVoices_[filename] = pSourceVoice;
}

// ==========================================================================
// 音声停止
// ==========================================================================
void SoundManager::StopWave(const std::string& filename){
    auto it = activeVoices_.find(filename);

    if(it != activeVoices_.end()){
        IXAudio2SourceVoice* pSourceVoice = it->second;

        pSourceVoice->Stop();               // 再生停止
        pSourceVoice->FlushSourceBuffers(); // 待機中バッファのクリア
        pSourceVoice->DestroyVoice();       // ボイスの破棄

        activeVoices_.erase(it);            // リストから削除
    }
}

// ==========================================================================
// 再生中チェック
// ==========================================================================
bool SoundManager::IsPlaying(const std::string& filename){
    return activeVoices_.find(filename) != activeVoices_.end();
}

// ==========================================================================
// 一時停止
// ==========================================================================
void SoundManager::PauseWave(const std::string& filename){
    auto it = activeVoices_.find(filename);
    if(it != activeVoices_.end()){
        it->second->Stop(); // Stopのみ呼ぶと位置を保持して止まる
    }
}

// ==========================================================================
// 再開
// ==========================================================================
void SoundManager::ResumeWave(const std::string& filename){
    auto it = activeVoices_.find(filename);
    if(it != activeVoices_.end()){
        it->second->Start(); // 続きから再生開始
    }
}