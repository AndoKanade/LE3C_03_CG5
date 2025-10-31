#pragma once
#include <Windows.h>
#include <cassert>
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800 // DirectInput version 8.0
#include "WinAPI.h"
#include <dinput.h>

class Input {
  // public:
  //   template <class T> using Comptr = Microsoft::WRL::ComPtr<T>;

public:
  void Initialize(WinAPI *winApi);
  void Update();

  bool PushKey(BYTE keyNumber);
  bool TriggerKey(BYTE keyNumber);

private:
  WinAPI *winApi_ = nullptr;

  Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
  Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;

  BYTE key[256] = {};
  BYTE preKey[256] = {};
};
