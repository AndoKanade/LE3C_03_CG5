#include "Input.h"

void Input::Initialize(WinAPI *winApi) {
  this->winApi_ = winApi;

  HRESULT result;

  result =
      DirectInput8Create(winApi->GetHinstance(), DIRECTINPUT_VERSION,
                         IID_IDirectInput8, (void **)&directInput, nullptr);
  assert(SUCCEEDED(result));

  result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  assert(SUCCEEDED(result));

  result = keyboard->SetDataFormat(&c_dfDIKeyboard);
  assert(SUCCEEDED(result));

  result = keyboard->SetCooperativeLevel(winApi->GetHwnd(),
                                         DISCL_FOREGROUND | DISCL_NONEXCLUSIVE |
                                             DISCL_NOWINKEY);
  assert(SUCCEEDED(result));
}

void Input::Update() {

  HRESULT result;

  memcpy(preKey, key, sizeof(key));

  result = keyboard->Acquire();
  result = keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE keyNumber) {
  if (key[keyNumber]) {
    return true;
  }

  return false;
}

bool Input::TriggerKey(BYTE keyNumber) {
  if (!preKey[keyNumber] && key[keyNumber]) {
    return true;
  }

  return false;
}
