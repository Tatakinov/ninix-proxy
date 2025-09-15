#define WIN32_LEAN_AND_MEAN

#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <io.h>
#include <iostream>
#include <string>
#include <windows.h>

using ShioriLoad = BOOL (__cdecl *)(HGLOBAL h, long len);
using ShioriUnLoad = BOOL (__cdecl *)();
using ShioriRequest = HGLOBAL (__cdecl *)(HGLOBAL h, long *len);

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }
    HMODULE m;
    ShioriLoad load = nullptr;
    ShioriLoad loadu = nullptr;
    ShioriUnLoad unload = nullptr;
    ShioriRequest request = nullptr;
    {
        std::string tmp = argv[1];
        std::u8string dll_path(tmp.begin(), tmp.end());
        fs::path p = dll_path;
        m = LoadLibraryW(p.wstring().c_str());
        if (m == nullptr) {
            std::cout << "false";
            return 1;
        }
        load = reinterpret_cast<ShioriLoad>(GetProcAddress(m, "load"));
        loadu = reinterpret_cast<ShioriLoad>(GetProcAddress(m, "loadu"));
        unload = reinterpret_cast<ShioriUnLoad>(GetProcAddress(m, "unload"));
        request = reinterpret_cast<ShioriRequest>(GetProcAddress(m, "request"));
        if (load == nullptr || unload == nullptr || request == nullptr) {
            FreeLibrary(m);
            std::cout << "false";
            return 1;
        }
    }
    if (argc < 3) {
        FreeLibrary(m);
        std::cout << "true";
        return 0;
    }
    {
        std::string tmp = argv[2];
        if (loadu == nullptr) {
            std::u8string ghost_dir(tmp.begin(), tmp.end());
            fs::path p = ghost_dir;
            std::string ansi = p.string();
            HGLOBAL h = GlobalAlloc(GPTR, ansi.length() + 1);
            if (h == nullptr) {
                FreeLibrary(m);
                return 1;
            }
            memmove(h, ansi.c_str(), ansi.length());
            if (!load(h, ansi.length())) {
                FreeLibrary(m);
                return 1;
            }
        }
        else {
            HGLOBAL h = GlobalAlloc(GPTR, tmp.length() + 1);
            if (h == nullptr) {
                FreeLibrary(m);
                return 1;
            }
            memmove(h, tmp.c_str(), tmp.length());
            if (!loadu(h, tmp.length())) {
                FreeLibrary(m);
                return 1;
            }
        }
    }
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    uint32_t len;
    long shiori_len;
    while (true) {
        std::cin.read(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        if (std::cin.eof() || len == 0) {
            break;
        }
        std::string req;
        req.resize(len);
        std::cin.read(req.data(), len);
        if (std::cin.gcount() < len) {
            FreeLibrary(m);
            return 1;
        }

        shiori_len = req.length();
        HGLOBAL h = GlobalAlloc(GPTR, shiori_len + 1);
        if (h == nullptr) {
            FreeLibrary(m);
            return 1;
        }
        memmove(h, req.c_str(), shiori_len);
        h = request(h, &shiori_len);
        if (h == nullptr) {
            len = 0;
            std::cout.write(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        }
        else {
            len = shiori_len;
            std::string response(reinterpret_cast<char *>(h), len);
            GlobalFree(h);
            std::cout.write(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            std::cout.write(response.c_str(), len);
        }
    }
    unload();
    FreeLibrary(m);
    return 0;
}
