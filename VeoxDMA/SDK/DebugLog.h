#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>
#include <string>
#include <windows.h>

namespace DebugLog {

inline std::mutex& Mutex() { static std::mutex m; return m; }

inline FILE* Open() {
    static FILE* f = nullptr;
    if (f) return f;
    std::wstring exePath(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, exePath.data(), static_cast<DWORD>(exePath.size()));
        if (length == 0) return nullptr;
        if (length < exePath.size()) {
            exePath.resize(length);
            break;
        }
        if (exePath.size() >= 32768) return nullptr;
        exePath.resize(exePath.size() * 2);
    }
    const auto sep = exePath.find_last_of(L"\\/");
    if (sep == std::wstring::npos) return nullptr;
    exePath.resize(sep + 1);
    exePath += L"debug.log";
    f = _wfopen(exePath.c_str(), L"a");
    if (f) {
        fprintf(f, "\n=== SESSION START (pid %lu) ===\n", GetCurrentProcessId());
        fflush(f);
    }
    return f;
}

inline void Log(const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(Mutex());
    FILE* f = Open();
    if (!f) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fprintf(f, "\n");
    fflush(f);
}

} // namespace DebugLog

#ifdef _DEBUG
#define DLOG(...) DebugLog::Log(__VA_ARGS__)
#else
#define DLOG(...) ((void)0)
#endif
