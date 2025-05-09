#include "pch.h"
#include "Utils.h"
#include <filesystem>
#include <windows.h>
#include <algorithm>

std::string WideCharToUTF8(const wchar_t* wideStr)
{
    // ��ȡת��������ֽ�����UTF-8��
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";

    // ���仺����
    std::string utf8Str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str.data(), size, nullptr, nullptr);

    // �Ƴ�ĩβ�Ŀ��ַ�������У�
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}
