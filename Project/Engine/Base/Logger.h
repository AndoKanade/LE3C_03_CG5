#pragma once
#include <string>
#include <Windows.h>

#pragma comment(lib, "Dbghelp.lib")

// namespace ではなく class にします
class Logger{
public:
    // static メンバ関数にすることで、インスタンスを作らずに
    // Logger::Log() のように呼べるようになります
    static void Log(const std::string& message);

    // こちらも static メンバ関数にします
    static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);
};