#define DLL_EXPORT __declspec(dllexport)

extern "C"
{
    bool DLL_EXPORT Start(const wchar_t* folderPath, const wchar_t* fileExtension, const bool ftpServerEnabled);
    void DLL_EXPORT Stop();
}
