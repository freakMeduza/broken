#pragma once

#include <memory>
#include <vector>

namespace broken {

struct resource_handle_base {
    uint64_t data;
    static constexpr uint32_t INDEX_BITS = 20;
    static constexpr uint64_t INDEX_MASK = (1ull << INDEX_BITS) - 1; // 0xFFFFF
    [[nodiscard]] inline uint32_t index() const noexcept { return static_cast<uint32_t>(data & INDEX_MASK); }
    [[nodiscard]] inline uint64_t generation() const noexcept { return data >> INDEX_BITS; }
    [[nodiscard]] inline bool valid() const noexcept { return generation() != 0; }
};

template <typename T>
struct resource_handle : public resource_handle_base {
    inline static resource_handle<T> make() noexcept { return make(0, 0); }
    inline static resource_handle<T> make(uint32_t index, uint64_t generation) noexcept {
        return {(static_cast<uint64_t>(index) & INDEX_MASK) | (generation << INDEX_BITS)};
    }
};

template <typename T, typename U>
class resource_registry {
public:
    explicit resource_registry(size_t size) {
        m_slots.resize(size);
        for (uint32_t i = 0; i < size; ++i) {
            m_free.push_back(i);
        }
    }

    using handle = resource_handle<T>;

    [[nodiscard]] inline handle registerResource(std::unique_ptr<U> resource) noexcept {
        if (m_free.empty()) {
            return handle::make();
        }

        const auto index = m_free.back();
        m_free.pop_back();

        if (index >= m_slots.size()) {
            return handle::make();
        }

        auto& slot = m_slots[index];
        slot.resource = std::move(resource);

        return handle::make(index, slot.generation);
    }

    inline std::unique_ptr<U> unregisterResource(handle handle) noexcept {
        const auto index = handle.index();
        if (index >= m_slots.size()) {
            return nullptr;
        }

        auto& slot = m_slots[index];
        if (slot.generation != handle.generation()) {
            return nullptr;
        }

        slot.generation++;
        if (slot.generation == 0) {
            slot.generation = 1;
        }

        m_free.push_back(index);

        return std::move(slot.resource);
    }

    [[nodiscard]] inline U* resource(handle handle) const noexcept {
        const auto index = handle.index();
        if (index >= m_slots.size()) {
            return nullptr;
        }

        auto& slot = m_slots[index];
        if (slot.generation != handle.generation()) {
            return nullptr;
        }

        return slot.resource.get();
    }

private:
    struct resource_slot {
        std::unique_ptr<U> resource;
        uint64_t generation = 1;
    };

    std::vector<resource_slot> m_slots;
    std::vector<uint32_t> m_free;
};

template <typename T, typename U, void (T::*DestroyFunc)(U*) const>
struct plugin_resource_deleter {
    const T* owner = nullptr;
    void operator()(U* resource) const noexcept {
        if (owner && resource) {
            (owner->*DestroyFunc)(resource);
        }
    }
};

} // namespace broken
