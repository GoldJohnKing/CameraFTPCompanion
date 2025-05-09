#define DLL_EXPORT __declspec(dllexport)

extern "C"
{
    bool DLL_EXPORT Start(const wchar_t* folderPath, const bool execEnabled, const wchar_t* execPath, const wchar_t* m_fileExtension, const int ftpPort);
    void DLL_EXPORT Stop();
}
