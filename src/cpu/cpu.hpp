#pragma once
#include <optional>

#include "../sor/core.hpp"
#include "instruction.hpp"
#include "memory.hpp"

namespace MiniCPU {
    using namespace JanSordid::Core;

    class CPU {
    public:
        void run(Memory& memory) noexcept;

    private:
        static constexpr u8 RegisterCount = 4;

        Array<u8, RegisterCount> _registers{};
        u8 _pc = 0;

        /*--- Fetch/Decode/Execute ---*/

        [[nodiscard]] u8 registerValue(Register reg) const noexcept;
        void setRegister(Register reg, u8 value) noexcept;

        [[nodiscard]] DecodedInstruction fetch(const Memory& memory) const noexcept;
        [[nodiscard]] ExecutionResult execute(const DecodedInstruction& instruction, Memory& memory) noexcept;

        /*--- Anzeige/Konsolen-Ausgabe ---*/

        [[nodiscard]] DynArray<String> buildInstructionLines(u32 step, u8 pcAtFetch, const DecodedInstruction& instruction,
            std::optional<u8> output, bool isStop) const noexcept;
        [[nodiscard]] DynArray<String> buildRegisterLines() const noexcept;
        [[nodiscard]] DynArray<String> buildRamLines(const Memory& memory, u8 highlightStart) const noexcept;

        void printStep(u32 step, u8 pcAtFetch, const DecodedInstruction& instruction, const Memory& memory,
            std::optional<u8> output, bool isStop) const noexcept;
        static void printSideBySide(const DynArray<String>& leftLines, const DynArray<String>& rightLines) noexcept;
    };
}
