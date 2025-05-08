#define DllExport __declspec(dllexport)

extern "C" DllExport bool Start(const wchar_t* folderPath, const wchar_t* fileExtension, const bool ftpServerEnabled = false);

extern "C" DllExport void Stop();
