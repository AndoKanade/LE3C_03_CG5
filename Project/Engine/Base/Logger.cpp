#include "Logger.h"
#include <debugapi.h> // OutputDebugStringA用
#include <dbghelp.h>
#include <strsafe.h>

#pragma comment(lib, "Dbghelp.lib")

// クラスのメンバ関数として定義する (namespace { } は不要)
void Logger::Log(const std::string& message){
	// シンプルにVisual Studioの出力ウィンドウに出す
	OutputDebugStringA(message.c_str());
}

// ダンプ出力機能
LONG WINAPI Logger::ExportDump(EXCEPTION_POINTERS* exception){
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = {0};

	// Dumpsフォルダ作成
	CreateDirectory(L"./Dumps",nullptr);

	// ファイル名決定
	StringCchPrintfW(filePath,MAX_PATH,L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute);

	// ファイル作成
	HANDLE dumpFileHandle = CreateFile(filePath,GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE | FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);

	MINIDUMP_EXCEPTION_INFORMATION minidumpinformation{0};
	minidumpinformation.ThreadId = GetCurrentThreadId();
	minidumpinformation.ExceptionPointers = exception;
	minidumpinformation.ClientPointers = TRUE;

	// ダンプ書き込み
	MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),dumpFileHandle,
		MiniDumpNormal,&minidumpinformation,nullptr,nullptr);

	// ハンドルを閉じるのを忘れずに
	CloseHandle(dumpFileHandle);

	return EXCEPTION_EXECUTE_HANDLER;
}