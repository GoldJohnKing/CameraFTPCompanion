#include "pch.h"
#include "Utils.h"
#include <filesystem>
#include <windows.h>
#include <algorithm>

std::string WideCharToUTF8(const wchar_t* wideStr)
{
    // 获取转换所需的字节数（UTF-8）
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";

    // 分配缓冲区
    std::string utf8Str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str.data(), size, nullptr, nullptr);

    // 移除末尾的空字符（如果有）
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}
