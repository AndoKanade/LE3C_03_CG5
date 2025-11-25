#define _USE_MATH_DEFINES
#define PI 3.14159265f
#include "DXCommon.h"
#include "Input.h"
#include "StringUtility.h"
#include "WinAPI.h"
#include <chrono>
#include <cmath>
#include <dbghelp.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string.h>
#include <strsafe.h>
#include <vector>
#include <xaudio2.h>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <iostream>
#include <map>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lPalam);

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace DirectX;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }

  switch (msg) {
  case WM_DESTROY:

    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

#pragma region 構造体
struct Vector2 {
  float x, y;
};

inline bool operator<(const Vector2 &a, const Vector2 &b) {
  if (a.x != b.x)
    return a.x < b.x;
  return a.y < b.y;
}

inline bool operator!=(const Vector2 &a, const Vector2 &b) {
  return a.x != b.x || a.y != b.y;
}

struct Vector3 {
  float x, y, z;
};

inline bool operator<(const Vector3 &a, const Vector3 &b) {
  if (a.x != b.x)
    return a.x < b.x;
  if (a.y != b.y)
    return a.y < b.y;
  return a.z < b.z;
}

inline bool operator!=(const Vector3 &a, const Vector3 &b) {
  return a.x != b.x || a.y != b.y || a.z != b.z;
}

struct Vector4 {
  float x, y, z, w;
};

inline bool operator<(const Vector4 &a, const Vector4 &b) {
  if (a.x != b.x)
    return a.x < b.x;
  if (a.y != b.y)
    return a.y < b.y;
  if (a.z != b.z)
    return a.z < b.z;
  return a.w < b.w;
}

inline bool operator!=(const Vector4 &a, const Vector4 &b) {
  return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
}
typedef struct Matrix4x4 {
  float m[4][4];
} Matrix4x4;

typedef struct Matrix3x3 {
  float m[3][3];
} Matrix3x3;

typedef struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
} Ttansform;

typedef struct TransformationMatrix {
  Matrix4x4 WVP;
  Matrix4x4 World;
} TransformationMatrix;

typedef struct Material {
  Vector4 color;
  int32_t enableLighting;
  float padding[3];
  Matrix4x4 uvTransform;
} Material;

struct VertexData {
  Vector4 position;
  Vector2 texcoord;
  Vector3 normal;

  bool operator<(const VertexData &other) const {
    if (position != other.position)
      return position < other.position;
    if (texcoord != other.texcoord)
      return texcoord < other.texcoord;
    if (normal != other.normal)
      return normal < other.normal;
    return false;
  }
};

struct DirectionalLight {
  Vector4 color;     // ライトの色
  Vector3 direction; // ライトの方向
  float intensity;   // ライトの強度
};

struct MaterialData {
  std::string textureFilePath;
};

struct ModelData {
  std::vector<VertexData> vertices;
  MaterialData material;
};

struct D3DResourceLeakChecker {
  ~D3DResourceLeakChecker() {

    Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
      debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
      debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
      debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
    }
  }
};

#pragma region サウンド再生
struct ChunkHeader {
  char id[4];
  int32_t size;
};

struct RiffHeader {
  ChunkHeader chunk;
  char type[4];
};

struct FormatChunk {
  ChunkHeader chunk;
  WAVEFORMATEX fmt;
};

struct SoundData {
  WAVEFORMATEX wfex;
  BYTE *pBUffer;
  unsigned int bufferSize;
  ;
};

#pragma endregion

struct WindowData {
  HINSTANCE hInstance;
  HWND hwnd;
  // 他にも必要な情報を含む
};

#pragma endregion

#pragma region 関数たち

#pragma region 音声データの読み込み

SoundData SoundLoadWave(const char *filename) {

  std::ifstream file;
  file.open(filename, std::ios_base::binary);
  assert(file.is_open());

  RiffHeader riff;
  file.read((char *)&riff, sizeof(riff));

  if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
    assert(0);
  }

  if (strncmp(riff.type, "WAVE", 4) != 0) {
    assert(0);
  }

  FormatChunk format = {};
  file.read((char *)&format, sizeof(ChunkHeader));

  if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
    assert(0);
  }

  assert(format.chunk.size <= sizeof(format.fmt));
  file.read((char *)&format.fmt, format.chunk.size);

  ChunkHeader data;
  file.read((char *)&data, sizeof(data));

  if (strncmp(data.id, "JUNK", 4) == 0) {
    file.seekg(data.size, std::ios_base::cur);

    file.read((char *)&data, sizeof(data));
  }

  if (strncmp(data.id, "data", 4) != 0) {
    assert(0);
  }

  char *pBuffer = new char[data.size];
  file.read(pBuffer, data.size);

  file.close();

  SoundData soundData = {};

  soundData.wfex = format.fmt;
  soundData.pBUffer = reinterpret_cast<BYTE *>(pBuffer);
  soundData.bufferSize = data.size;

  return soundData;
}

#pragma endregion

#pragma region 音声データの解放
void SoundUnload(SoundData *soundData) {
  delete[] soundData->pBUffer;

  soundData->pBUffer = 0;
  soundData->bufferSize = 0;
  soundData->wfex = {};
}

#pragma endregion

#pragma region サウンドの再生

void SoundPlayWave(IXAudio2 *xAudio2, const SoundData &soundData) {
  HRESULT result;

  IXAudio2SourceVoice *pSourceVoice = nullptr;
  result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
  assert(SUCCEEDED(result));

  XAUDIO2_BUFFER buf{};

  buf.pAudioData = soundData.pBUffer;
  buf.AudioBytes = soundData.bufferSize;
  buf.Flags = XAUDIO2_END_OF_STREAM;

  result = pSourceVoice->SubmitSourceBuffer(&buf);
  result = pSourceVoice->Start();
}

#pragma endregion

#pragma region log関数

// ログファイルの方
void Log(std::ostream &os, const std::string &message) {
  os << message << std::endl;
  OutputDebugStringA(message.c_str());
}

//
void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }
#pragma endregion

#pragma region ダンプ
static LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {

  SYSTEMTIME time;
  GetLocalTime(&time);
  wchar_t filePath[MAX_PATH] = {0};
  CreateDirectory(L"./Dumps", nullptr);
  StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
                   time.wYear, time.wMonth, time.wDay, time.wHour,
                   time.wMinute);
  HANDLE dumpFileHandle =
      CreateFile(filePath, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  // processIdとクラッシュの発生したthreadIdを取得
  DWORD processId = GetCurrentProcessId();
  DWORD threadId = GetCurrentThreadId();

  // 設定情報を入力
  MINIDUMP_EXCEPTION_INFORMATION minidumpinformation{0};
  minidumpinformation.ThreadId = threadId;
  minidumpinformation.ExceptionPointers = exception;
  minidumpinformation.ClientPointers = TRUE;

  // Dumpを出力、MiniDumpWriteNormalは最低限の情報を出力するフラグ
  MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
                    MiniDumpNormal, &minidumpinformation, nullptr, nullptr);

  return EXCEPTION_EXECUTE_HANDLER;
}
#pragma endregion

#pragma region MaterialTemplate関数
MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename) {
  MaterialData materialData;
  std::string line;
  std::ifstream file(directoryPath + "/" + filename);

  assert(file.is_open());

  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);

    s >> identifier;

    if (identifier == "map_Kd") {
      std::string textureFilename;
      s >> textureFilename;

      materialData.textureFilePath = directoryPath + "/" + textureFilename;
    }
  }
  return materialData;
}
#pragma endregion

#pragma region Objファイルを読む関数

ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename) {
  ModelData modelData;
  std::vector<Vector4> positions;
  std::vector<Vector3> normals;
  std::vector<Vector2> texcoords;

  std::ifstream file(directoryPath + "/" + filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open OBJ file: " << directoryPath + "/" + filename
              << std::endl;
    return {};
  }

  std::string line;

  while (std::getline(file, line)) {
    std::istringstream s(line);
    std::string identifier;
    s >> identifier;

    if (identifier == "v") {
      Vector4 position{};

      s >> position.x >> position.y >> position.z;

      position.w = 1.0f;
      //  position.x *= -1.0f;
      positions.push_back(position);

    } else if (identifier == "vt") {
      Vector2 texcoord{};
      s >> texcoord.x >> texcoord.y;

      texcoord.y = 1.0f - texcoord.y;
      texcoords.push_back(texcoord);

    } else if (identifier == "vn") {

      Vector3 normal{};

      s >> normal.x >> normal.y >> normal.z;
      //  normal.x *= -1.0f;
      normals.push_back(normal);

    } else if (identifier == "f") {

      VertexData triangle[3] = {};

      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vdef;
        s >> vdef;

        std::istringstream vstream(vdef);
        uint32_t elementIndices[3];

        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(vstream, index, '/');
          elementIndices[element] = std::stoi(index);
        }

        Vector4 position = positions[elementIndices[0] - 1];
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        Vector3 normal = normals[elementIndices[2] - 1];

        triangle[faceVertex] = {position, texcoord, normal};
      }
      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);

    } else if (identifier == "mtllib") {
      std::string materialFilename;
      s >> materialFilename;

      modelData.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }
  file.close();
  return modelData;
}

#pragma endregion

#pragma endregion

#pragma region 数学関数

Matrix4x4 MakeIdentity4x4() {
  Matrix4x4 result;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        result.m[i][j] = 1.0f;
      } else {
        result.m[i][j] = 0.0f;
      }
    }
  }
  return result;
}
Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {
  Matrix4x4 matrix = {}; // すべて0で初期化
  // スケール行列の設定
  matrix.m[0][0] = scale.x;
  matrix.m[1][1] = scale.y;
  matrix.m[2][2] = scale.z;
  matrix.m[3][3] = 1.0f;
  return matrix;
}
Matrix4x4 MakeTranslateMatrix(const Vector3 &translate) {
  Matrix4x4 matrix = {}; // すべて0で初期化
  // 単位行列の形に設定
  matrix.m[0][0] = 1.0f;
  matrix.m[1][1] = 1.0f;
  matrix.m[2][2] = 1.0f;
  matrix.m[3][3] = 1.0f;
  // 平行移動成分を設定
  matrix.m[3][0] = translate.x;
  matrix.m[3][1] = translate.y;
  matrix.m[3][2] = translate.z;
  return matrix;
}
Matrix4x4 MakeRotateXMatrix(float radian) {
  Matrix4x4 result{};

  result.m[0][0] = 1;
  result.m[3][3] = 1;

  // X軸回転に必要な部分だけ上書き
  result.m[1][1] = std::cos(radian);
  result.m[1][2] = std::sin(radian);
  result.m[2][1] = -std::sin(radian);
  result.m[2][2] = std::cos(radian);

  return result;
}
Matrix4x4 MakeRotateYMatrix(float radian) {
  Matrix4x4 result{};

  result.m[1][1] = 1.0f;
  result.m[3][3] = 1.0f;

  result.m[0][0] = std::cos(radian);
  result.m[0][2] = -std::sin(radian);
  result.m[2][0] = std::sin(radian);
  result.m[2][2] = std::cos(radian);

  return result;
}
Matrix4x4 MakeRotateZMatrix(float radian) {
  Matrix4x4 result{};

  result.m[2][2] = 1;
  result.m[3][3] = 1;

  result.m[0][0] = std::cos(radian);
  result.m[0][1] = std::sin(radian);
  result.m[1][0] = -std::sin(radian);
  result.m[1][1] = std::cos(radian);

  return result;
}

Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  Matrix4x4 result{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      result.m[row][col] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[row][col] += m1.m[row][k] * m2.m[k][col];
      }
    }
  }
  return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                           const Vector3 &translate) {

  Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
  Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
  Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
  Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);

  // 回転順: Z → X → Y →（スケーリング）→ 平行移動
  Matrix4x4 rotateMatrix = Multiply(Multiply(rotateX, rotateY), rotateZ);

  Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

  Matrix4x4 affineMatrix =
      Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

  return affineMatrix;
}
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {

  float f = 1.0f / tanf(fovY * 0.5f);
  float range = farClip / (farClip - nearClip);

  Matrix4x4 result = {};

  result.m[0][0] = f / aspectRatio;
  result.m[1][1] = f;
  result.m[2][2] = range;
  result.m[2][3] = 1.0f;
  result.m[3][2] = -range * nearClip; // ← DirectX ではマイナス
  result.m[3][3] = 0.0f;

  return result;
}

Matrix4x4 Inverse(const Matrix4x4 &m) {
  Matrix4x4 result{};

  // 行列の行列式を計算
  float det =
      m.m[0][0] *
          (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
           m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
           m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) -
      m.m[0][1] *
          (m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
           m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
           m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) +
      m.m[0][2] *
          (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
           m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
           m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) -
      m.m[0][3] * (m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
                   m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
                   m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

  if (det == 0) {
    // 行列式がゼロの場合、逆行列は存在しません
    return result; // 逆行列は存在しないのでゼロ行列を返す
  }

  // 行列式の逆数を計算
  float invDet = 1.0f / det;

  // 各要素を余因子行列から計算
  result.m[0][0] =
      (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
       m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
       m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) *
      invDet;
  result.m[0][1] =
      (-m.m[0][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) +
       m.m[0][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
       m.m[0][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) *
      invDet;
  result.m[0][2] =
      (m.m[0][1] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) -
       m.m[0][2] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) +
       m.m[0][3] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1])) *
      invDet;
  result.m[0][3] =
      (-m.m[0][1] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) +
       m.m[0][2] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) -
       m.m[0][3] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1])) *
      invDet;

  result.m[1][0] =
      (-m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) +
       m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) -
       m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) *
      invDet;
  result.m[1][1] =
      (m.m[0][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
       m.m[0][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
       m.m[0][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) *
      invDet;
  result.m[1][2] =
      -(m.m[0][0] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) -
        m.m[0][2] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) +
        m.m[0][3] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0])) *
      invDet;

  result.m[1][3] =
      (m.m[0][0] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) -
       m.m[0][2] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) +
       m.m[0][3] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0])) *
      invDet;

  result.m[2][0] =
      (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
       m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
       m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
      invDet;

  result.m[2][1] =
      (-m.m[0][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
       m.m[0][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) -
       m.m[0][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
      invDet;

  result.m[2][2] =
      (m.m[0][0] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) -
       m.m[0][1] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) +
       m.m[0][3] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0])) *
      invDet;

  result.m[2][3] =
      (-m.m[0][0] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) +
       m.m[0][1] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) -
       m.m[0][3] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0])) *
      invDet;

  result.m[3][0] =
      (-m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) +
       m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) -
       m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
      invDet;

  result.m[3][1] =
      (m.m[0][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
       m.m[0][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
       m.m[0][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
      invDet;

  result.m[3][2] =
      (-m.m[0][0] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1]) +
       m.m[0][1] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0]) -
       m.m[0][2] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0])) *
      invDet;

  result.m[3][3] =
      (m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) -
       m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) +
       m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0])) *
      invDet;

  return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
  Matrix4x4 result = {};

  result.m[0][0] = 2.0f / (right - left);
  result.m[1][1] = 2.0f / (top - bottom);
  result.m[2][2] = 1.0f / (farClip - nearClip);
  result.m[3][0] = (left + right) / (left - right);
  result.m[3][1] = (top + bottom) / (bottom - top);
  result.m[3][2] = -nearClip / (farClip - nearClip);
  result.m[3][3] = 1.0f;

  return result;
}

#pragma endregion

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  D3DResourceLeakChecker leakCheck;

  WinAPI *winApi = nullptr;

  Input *input = nullptr;

  Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
  IXAudio2MasteringVoice *masterVoice;

  // 例外が発生したらダンプを出力する
  SetUnhandledExceptionFilter(ExportDump);

  winApi = new WinAPI();
  winApi->Initialize();

  DXCommon *dxCommon = nullptr;
  dxCommon = new DXCommon();
  dxCommon->Initialize(winApi);

  HRESULT hr;

  input = new Input();
  input->Initialize(winApi);

  SoundData soundData = SoundLoadWave("resource/You_and_Me.wav");
  bool hasPlayed = false;

#pragma region DepthStencil

  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

  depthStencilDesc.DepthEnable = true;
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

#pragma endregion

#pragma region ルートシグネチャーを生成する

  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};

  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

#pragma region RootParameter

  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0;
  descriptorRange[0].NumDescriptors = 1;
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER rootParameters[4] = {};

  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameters[0].Descriptor.ShaderRegister = 0;

  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  rootParameters[1].Descriptor.ShaderRegister = 1;

  rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange);

  rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameters[3].Descriptor.ShaderRegister = 2;

  descriptionRootSignature.pParameters = rootParameters;
  descriptionRootSignature.NumParameters = _countof(rootParameters);

#pragma endregion

#pragma region Sampler

  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};

  staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  staticSamplers[0].MinLOD = 0.0f;
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
  staticSamplers[0].ShaderRegister = 0;
  staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  descriptionRootSignature.pStaticSamplers = staticSamplers;
  descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

#pragma endregion

  Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
  hr = D3D12SerializeRootSignature(&descriptionRootSignature,
                                   D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
                                   &errorBlob);

  if (FAILED(hr)) {
    Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
    assert(false);
  }

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
  hr = dxCommon->GetDevice()->CreateRootSignature(
      0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&rootSignature));
  assert(SUCCEEDED(hr));

#pragma endregion

#pragma region InputLayoutを生成する

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
  inputElementDescs[0].SemanticName = "POSITION";
  inputElementDescs[0].SemanticIndex = 0;
  inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[1].SemanticName = "TEXCOORD";
  inputElementDescs[1].SemanticIndex = 0;
  inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[2].SemanticName = "NORMAL";
  inputElementDescs[2].SemanticIndex = 0;
  inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
  inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = inputElementDescs;
  inputLayoutDesc.NumElements = _countof(inputElementDescs);

#pragma endregion

#pragma region BlendStateを生成する

  D3D12_BLEND_DESC blendDesc{};

  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;
  blendDesc.RenderTarget[0].BlendEnable = true;
  blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

#pragma endregion

#pragma region RasterrizerStateを生成する

  D3D12_RASTERIZER_DESC resterizerDesc{};
  resterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  resterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#pragma endregion

#pragma region shaderをコンパイルする

  Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
      dxCommon->CompileShader(L"resource/shader/Object3d.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
      dxCommon->CompileShader(L"resource/shader/Object3d.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

#pragma endregion

#pragma region PSOを生成する

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
  graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
  graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
  graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                  vertexShaderBlob->GetBufferSize()};
  graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                  pixelShaderBlob->GetBufferSize()};
  graphicsPipelineStateDesc.BlendState = blendDesc;
  graphicsPipelineStateDesc.RasterizerState = resterizerDesc;

  // 書き込むRTVの情報
  graphicsPipelineStateDesc.NumRenderTargets = 1;
  graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

  graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // 利用する形状のタイプ(今回は三角形)
  graphicsPipelineStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  //  どのような画面に色を打ち込むかの設定
  graphicsPipelineStateDesc.SampleDesc.Count = 1;
  graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  //  実際に生成
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
  hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
  assert(SUCCEEDED(hr));

#pragma endregion

#pragma region VertexResourceを生成する

  // 分割数（自由に調整可能）
  const int kLatitudeDiv = 16;  // 縦（経度）
  const int kLongitudeDiv = 16; // 横（緯度）

  int sphereVertexCount = kLatitudeDiv * kLongitudeDiv * 6;

  ModelData modelData = LoadObjFile("resource", "plane.obj");

  // VertexResource を生成
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource =
      dxCommon->CreateBufferResource(sizeof(VertexData) *
                                     modelData.vertices.size());

  // Spriteの矩形
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite =
      dxCommon->CreateBufferResource(sizeof(VertexData) * 6);

#pragma endregion

#pragma region IndexResourceを生成する

  Microsoft::WRL::ComPtr<ID3D12Resource> indexResource =
      dxCommon->CreateBufferResource(sizeof(uint32_t) *
                                     modelData.vertices.size());

  D3D12_INDEX_BUFFER_VIEW indexBufferView{};
  indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
  indexBufferView.SizeInBytes =
      UINT(sizeof(uint32_t) * modelData.vertices.size());
  indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite =
      dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);

  D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

  indexBufferViewSprite.BufferLocation =
      indexResourceSprite->GetGPUVirtualAddress();
  indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
  indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

#pragma endregion

#pragma region MaterialResource

  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource =
      dxCommon->CreateBufferResource(sizeof(Material));
  Material *materialData = nullptr;

  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
  *materialData = Material{Vector4(1.0f, 1.0f, 1.0f, 1.0f), 1};
  materialData->enableLighting = true;
  materialData->uvTransform = MakeIdentity4x4();

  Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite =
      dxCommon->CreateBufferResource(sizeof(Material));
  Material *materialDataSprite = nullptr;
  materialResourceSprite->Map(0, nullptr,
                              reinterpret_cast<void **>(&materialDataSprite));
  *materialDataSprite = Material{Vector4(1.0f, 1.0f, 1.0f, 1.0f), 0};

  materialDataSprite->enableLighting = false;
  materialDataSprite->uvTransform = MakeIdentity4x4();

#pragma endregion

#pragma region directionalLight

  Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =
      dxCommon->CreateBufferResource(sizeof(DirectionalLight));
  DirectionalLight *directionalLightData = nullptr;
  directionalLightResource->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData));

  directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData->direction = {0.0f, 0.0f, -1.0f};
  directionalLightData->intensity = 1.0f;

#pragma endregion

#pragma region TransformationMatrix用のResourceを作る

  Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource =
      dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
  TransformationMatrix *wvpData = nullptr;

  wvpResource->Map(0, nullptr, reinterpret_cast<void **>(&wvpData));

  // Sprite

  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite =
      dxCommon->CreateBufferResource(sizeof(Matrix4x4));

  Matrix4x4 *transformationMatrixDataSprite = nullptr;
  transformationMatrixResourceSprite->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSprite));
  *transformationMatrixDataSprite = MakeIdentity4x4();
  transformationMatrixResourceSprite->Unmap(0, nullptr);

#pragma endregion

#pragma region VertexBufferViewを生成する

  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

  vertexBufferView.SizeInBytes =
      UINT(sizeof(VertexData) * modelData.vertices.size());

  vertexBufferView.StrideInBytes = sizeof(VertexData);

  // Sprite
  D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
  vertexBufferViewSprite.BufferLocation =
      vertexResourceSprite->GetGPUVirtualAddress();
  vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
  vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

#pragma endregion

#pragma region Textureの読み込み

  DirectX::ScratchImage mipImages =
      dxCommon->LoadTexture("resource/uvChecker.png");
  const DirectX::TexMetadata metadata = mipImages.GetMetadata();
  Microsoft::WRL::ComPtr<ID3D12Resource> textureResource =
      dxCommon->CreateTextureResource(metadata);
  Microsoft::WRL::ComPtr<ID3D12Resource> intermadiate =
      dxCommon->UploadTextureData(textureResource, mipImages);

  DirectX::ScratchImage mipImages2 =
      dxCommon->LoadTexture(modelData.material.textureFilePath);
  const DirectX::TexMetadata &metadata2 = mipImages2.GetMetadata();
  Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 =
      dxCommon->CreateTextureResource(metadata2);
  Microsoft::WRL::ComPtr<ID3D12Resource> intermadiate2 =
      dxCommon->UploadTextureData(textureResource2, mipImages2);

#pragma endregion

#pragma region SRVを生成する

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

  //  ２枚目
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};

  srvDesc2.Format = metadata.format;
  srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

  // SRVを作成するDescriptorHeapの場所を決める

  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
      dxCommon->GetSRVCPUDescriptorHandle(0);

  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
      dxCommon->GetSRVGPUDescriptorHandle(0);

  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 =
      dxCommon->GetSRVCPUDescriptorHandle(2);

  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 =
      dxCommon->GetSRVGPUDescriptorHandle(2);

  textureSrvHandleCPU.ptr +=
      dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  textureSrvHandleGPU.ptr +=
      dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  //  SRVを生成する
  dxCommon->GetDevice()->CreateShaderResourceView(
      textureResource.Get(), &srvDesc, textureSrvHandleCPU);

  //   SRVを生成する
  dxCommon->GetDevice()->CreateShaderResourceView(
      textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

#pragma endregion

#pragma region 頂点データの更新

  // const uint32_t kSubdivision = 16;
  // const float kLonEvery = 2.0f * PI / float(kSubdivision);
  // const float kLatEvery = PI / float(kSubdivision);
  // const float epsilon = 1e-5f;

  VertexData *vertexData = nullptr;
  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  std::memcpy(vertexData, modelData.vertices.data(),
              sizeof(VertexData) * modelData.vertices.size());

  // vertexResource->Unmap(0, nullptr);

  //// 頂点生成（(kSubdivision + 1)^2 個）
  // for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
  //   float lat = -PI / 2.0f + kLatEvery * latIndex;
  //   float v = 1.0f - float(latIndex) / float(kSubdivision);

  //  for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
  //    float lon = lonIndex * kLonEvery;
  //    float u = float(lonIndex) / float(kSubdivision);
  //    if (lonIndex == kSubdivision) {
  //      u = 1.0f - epsilon; // wrap防止
  //    }

  //    Vector3 pos = {cosf(lat) * cosf(lon), sinf(lat), cosf(lat) * sinf(lon)};

  //    uint32_t vertexIndex = latIndex * (kSubdivision + 1) + lonIndex;

  //    vertexData[vertexIndex].position = {pos.x, pos.y, pos.z, 1.0f};
  //    vertexData[vertexIndex].normal = pos;
  //    vertexData[vertexIndex].texcoord = {u, v};
  //  }
  //}

  // uint32_t *indexData = nullptr;
  // indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));

  // uint32_t index = 0;
  // for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
  //   for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
  //     uint32_t i0 = latIndex * (kSubdivision + 1) + lonIndex;
  //     uint32_t i1 = i0 + 1;
  //     uint32_t i2 = i0 + (kSubdivision + 1);
  //     uint32_t i3 = i2 + 1;

  //    // 三角形 1
  //    indexData[index++] = i0;
  //    indexData[index++] = i2;
  //    indexData[index++] = i1;

  //    // 三角形 2
  //    indexData[index++] = i1;
  //    indexData[index++] = i2;
  //    indexData[index++] = i3;
  //  }
  //}

  uint32_t *indexData = nullptr;
  indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
  std::memcpy(indexData, modelData.vertices.data(),
              sizeof(uint32_t) * modelData.vertices.size());

#pragma region 画像データの頂点データ
  VertexData *vertexDataSprite = nullptr;
  vertexResourceSprite->Map(0, nullptr,
                            reinterpret_cast<void **>(&vertexDataSprite));

  //  矩形の4頂点（左上 → 左下 → 右上 → 右下）
  vertexDataSprite[0].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
  vertexDataSprite[0].texcoord = {0.0f, 0.0f};
  vertexDataSprite[0].normal = {0.0f, 0.0f, -1.0f};

  vertexDataSprite[1].position = {0.0f, 360.0f, 0.0f, 1.0f}; // 左下
  vertexDataSprite[1].texcoord = {0.0f, 1.0f};
  vertexDataSprite[1].normal = {0.0f, 0.0f, -1.0f};

  vertexDataSprite[2].position = {640.0f, 0.0f, 0.0f, 1.0f}; // 右上
  vertexDataSprite[2].texcoord = {1.0f, 0.0f};
  vertexDataSprite[2].normal = {0.0f, 0.0f, -1.0f};

  vertexDataSprite[3].position = {640.0f, 360.0f, 0.0f, 1.0f}; // 右下
  vertexDataSprite[3].texcoord = {1.0f, 1.0f};
  vertexDataSprite[3].normal = {0.0f, 0.0f, -1.0f};

#pragma endregion

#pragma region インデックスデータの生成

  uint32_t *indexDataSprite = nullptr;
  indexResourceSprite->Map(0, nullptr,
                           reinterpret_cast<void **>(&indexDataSprite));

  // 三角形1: 左上 → 左下 → 右上
  indexDataSprite[0] = 0;
  indexDataSprite[1] = 2;
  indexDataSprite[2] = 1;

  //  三角形2: 左下 → 右下 → 右上
  indexDataSprite[3] = 1;
  indexDataSprite[4] = 2;
  indexDataSprite[5] = 3;

#pragma endregion

#pragma endregion

#pragma region XAudio2の初期化
  HRESULT result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
  result = xAudio2->CreateMasteringVoice(&masterVoice);
  assert(SUCCEEDED(result));
#pragma endregion

#pragma region 変数宣言
  Transform transform{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},

  };

  Transform transformSprite{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  Transform cameraTransform{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

  Transform uvTransformSprite{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f, float(WinAPI::kCliantWidth) / float(WinAPI::kCliantHeight), 0.1f,
      100.0f);

#pragma region ImGuiの初期化

  // 三角形の初期値
  Vector4 triangleColor = {1.0f, 1.0f, 1.0f, 1.0f};

  Material *material = nullptr;

  bool useMonsterBall = false;

#pragma endregion

#pragma endregion
  while (true) {

    if (winApi->ProcessMessage()) {
      /// trueを返したときはゲームループを抜ける
      break;
    } else {
      // ゲームの処理

      // if (!hasPlayed) {
      //   SoundPlayWave(xAudio2.Get(), soundData);
      //   hasPlayed = true;
      // }

#ifdef _DEBUG
      ImGui_ImplDX12_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("Settings");

      // === Triangle Color ===
      if (ImGui::CollapsingHeader("model Color")) {
        ImGui::ColorEdit4("Color", reinterpret_cast<float *>(&triangleColor));
      }

      if (ImGui::CollapsingHeader("CameraTransform")) {
        ImGui::DragFloat3("CameraTranslate", &cameraTransform.translate.x,
                          0.1f);
        ImGui::SliderAngle("CameraRotateX", &cameraTransform.rotate.x);
        ImGui::SliderAngle("CameraRotateY", &cameraTransform.rotate.y);
        ImGui::SliderAngle("CameraRotateZ", &cameraTransform.rotate.z);
      }

      // === Transform (SRT Controller) ===
      if (ImGui::CollapsingHeader("modelTransform")) {
        ImGui::SliderAngle("RotateX", &transform.rotate.x);
        ImGui::SliderAngle("RotateY", &transform.rotate.y);
        ImGui::SliderAngle("RotateZ", &transform.rotate.z);
        // ImGui::Checkbox("useMonsterBall", &useMonsterBall);
        // materialData->enableLighting = useMonsterBall;
      }

      if (ImGui::CollapsingHeader("Light Settings")) {
        ImGui::Text("Directional Light");

        ImGui::ColorEdit4(
            "Color", reinterpret_cast<float *>(&directionalLightData->color));

        ImGui::SliderFloat3(
            "Direction",
            reinterpret_cast<float *>(&directionalLightData->direction), -1.0f,
            1.0f);

        // Normalize direction
        Vector3 &v = directionalLightData->direction;
        XMVECTOR dir = XMVectorSet(v.x, v.y, v.z, 0.0f);
        dir = XMVector3Normalize(dir);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3 *>(&v), dir);

        ImGui::SliderFloat("Intensity", &directionalLightData->intensity, 0.0f,
                           10.0f);
      }

      // === Sprite Transform ===
      if (ImGui::CollapsingHeader("Sprite Transform")) {
        ImGui::DragFloat3("Scale##Sprite", &transformSprite.scale.x, 0.1f, 0.1f,
                          10.0f);
        ImGui::DragFloat3("Rotate (rad)##Sprite", &transformSprite.rotate.x,
                          0.01f, -3.14f, 3.14f);
        ImGui::DragFloat3("Translate##Sprite", &transformSprite.translate.x,
                          1.0f, -700.0f, 700.0f);
      }

      // === UV Transform Sprite ===
      if (ImGui::CollapsingHeader("UV Transform Sprite")) {
        ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f,
                          -10.0f, 10.0f);
        ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f,
                          10.0f);
        ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
      }

      ImGui::End();
      ImGui::Render();

#endif

      ///================================
      /// 更新処理
      /// ==============================

      Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);

      uvTransformMatrix = Multiply(
          uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
      uvTransformMatrix = Multiply(
          uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
      materialDataSprite->uvTransform = uvTransformMatrix;

      /// Sprite用のWorldViewProjectionMatrixを作る

      Matrix4x4 worldMatrixSprite =
          MakeAffineMatrix(transformSprite.scale, transformSprite.rotate,
                           transformSprite.translate);
      Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
      Matrix4x4 projectionMatrixSprite =
          MakeOrthographicMatrix(0.0f, 0.0f, float(WinAPI::kCliantWidth),
                                 float(WinAPI::kCliantHeight), 0.0f, 100.0f);
      Matrix4x4 worldViewProjectionMatrixSprite =
          Multiply(projectionMatrixSprite,
                   Multiply(viewMatrixSprite, worldMatrixSprite));
      *transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

#pragma region 三角形の回転

      Matrix4x4 worldMatrix = MakeAffineMatrix(
          transform.scale, transform.rotate, transform.translate);
      Matrix4x4 cameraMatrix =
          MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
                           cameraTransform.translate);
      Matrix4x4 viewMatrix = Inverse(cameraMatrix);
      Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
          0.45f, float(WinAPI::kCliantWidth) / float(WinAPI::kCliantHeight),
          0.1f, 100.0f);
      Matrix4x4 worldViewProjectionMatrix =
          Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

      //     transform.rotate.y += 0.01f;
      *wvpData = {worldViewProjectionMatrix, worldMatrix};

#pragma endregion
      input->Update();

      if (input->TriggerKey(DIK_SPACE)) {
        OutputDebugStringA("Trigger space\n");
      }

      ///================================
      /// 描画処理
      ///================================

      dxCommon->PreDraw();

      dxCommon->commandList.Get()->SetGraphicsRootSignature(
          rootSignature.Get());
      dxCommon->commandList.Get()->SetPipelineState(
          graphicsPipelineState.Get());

      // 三角形の色を変える
      materialResource->Map(0, nullptr, reinterpret_cast<void **>(&material));
      material->color = triangleColor;
      materialResource->Unmap(0, nullptr);

      // 球の描画：Material, WVP, Light を正しくセット
      dxCommon->commandList.Get()->IASetVertexBuffers(0, 1, &vertexBufferView);
      dxCommon->commandList.Get()->IASetIndexBuffer(&indexBufferView);
      dxCommon->commandList.Get()->IASetPrimitiveTopology(
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
          0, materialResource->GetGPUVirtualAddress());
      dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
          1, wvpResource->GetGPUVirtualAddress());
      dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
          3, directionalLightResource->GetGPUVirtualAddress());
      dxCommon->commandList.Get()->SetGraphicsRootDescriptorTable(
          2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
      //     uint32_t indexCount = kSubdivision * kSubdivision * 6;

      dxCommon->commandList.Get()->DrawInstanced(
          UINT(modelData.vertices.size()), 1, 0, 0);

      dxCommon->commandList.Get()->IASetVertexBuffers(0, 1,
                                                      &vertexBufferViewSprite);
      dxCommon->commandList.Get()->IASetIndexBuffer(&indexBufferViewSprite);
      dxCommon->commandList.Get()->IASetPrimitiveTopology(
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
          0, materialResourceSprite->GetGPUVirtualAddress());
      dxCommon->commandList.Get()->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
      dxCommon->commandList.Get()->SetGraphicsRootDescriptorTable(
          2, textureSrvHandleGPU);

      //  commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

      ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                    dxCommon->commandList.Get());
      dxCommon->PostDraw();

#pragma endregion
    }
  }

#pragma region ImGuiの終了処理
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
#pragma endregion

  Log("unkillable demon king\n");


  delete dxCommon;
  dxCommon = nullptr;

  delete input;
  input = nullptr;

  xAudio2.Reset();
  SoundUnload(&soundData);

  //  CloseHandle(fenceEvent);

  winApi->Finalize();
  delete winApi;
  winApi = nullptr;

  return 0;
}