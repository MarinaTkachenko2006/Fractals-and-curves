#include "dialog_windows.h"

void FileDialogState::reset() noexcept { pending = false; ready = false; ok = false; path.clear(); }
void SaveDialogState::reset() noexcept { pending = false; ready = false; ok = false; path.clear(); }

void SDLCALL onFileDialogResult(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* st = static_cast<FileDialogState*>(userdata);

    st->pending = false;
    st->ready = true;

    if (!filelist || !filelist[0]) { st->ok = false; return; }

    st->ok = true;
    st->path = filelist[0];
}

void SDLCALL onSaveFileDialogResult(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* st = static_cast<SaveDialogState*>(userdata);

    st->pending = false;
    st->ready = true;

    if (!filelist || !filelist[0]) { st->ok = false; return; }

    st->ok = true;
    st->path = filelist[0];
}