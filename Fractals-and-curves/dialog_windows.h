#ifndef __DIALOG_WINDOWS_IMD_MAR_VIK__
#define __DIALOG_WINDOWS_IMD_MAR_VIK__

#include <SDL3/SDL.h>
#include <string>

struct FileDialogState {
    bool        pending = false;
    bool        ready = false;
    bool        ok = false;
    std::string path;

    void reset() noexcept;
};

struct SaveDialogState {
    bool        pending = false;
    bool        ready = false;
    bool        ok = false;
    std::string path;

    void reset() noexcept;
};

void SDLCALL onFileDialogResult(void* userdata, const char* const* filelist, int filter);
void SDLCALL onSaveFileDialogResult(void* userdata, const char* const* filelist, int filter);

#endif // !__DIALOG_WINDOWS_IMD_MAR_VIK__