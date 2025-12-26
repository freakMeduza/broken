#include "dynamic_library.hpp"

#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#error "Platfrom is not supported!"
#endif

namespace broken {

DynamicLibrary::DynamicLibrary(const std::string& fileName) {
    m_handle = LoadLibrary(fileName.c_str());
}

DynamicLibrary::~DynamicLibrary() noexcept {
    FreeLibrary((HMODULE)m_handle);
}

void* DynamicLibrary::getSymbol(const std::string& symbolName) const {
    return m_handle ? GetProcAddress((HMODULE)m_handle, symbolName.c_str()) : nullptr;
}

} // namespace broken
