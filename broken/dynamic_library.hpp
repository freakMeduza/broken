#pragma once

#include <string>

namespace broken {

class DynamicLibrary final {
public:
    explicit DynamicLibrary(const std::string& fileName);

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    ~DynamicLibrary() noexcept;

    void* getSymbol(const std::string& symbolName) const;

    template <typename T> inline T getSymbol(const std::string& symbolName) const {
        return reinterpret_cast<T>(getSymbol(symbolName));
    }

private:
    void* m_handle = nullptr;
};

} // namespace broken
