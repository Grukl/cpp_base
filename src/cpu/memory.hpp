#pragma once
#include "../sor/core.hpp"

namespace MiniCPU {
    using namespace JanSordid::Core;

    class Memory {
    public:
        static constexpr usize Size = 256;

        [[nodiscard]] u8 read(u8 addr) const noexcept;
        void write(u8 addr, u8 value) noexcept;
        void load(const DynArray<u8>& bytes) noexcept;

    private:
        Array<u8, Size> _data{};
    };
}