#pragma once
#include <optional>
#include "../sor/core.hpp"

namespace MiniCPU {
    using namespace JanSordid::Core;

    static_assert(sizeof(u8) == 1, "u8 must be exactly 1 byte");

    enum class Opcode : u8 {
        Stop = 0x00,
        LoadImm = 0x01,
        LoadMem = 0x02,
        Store = 0x03,
        Add = 0x04,
        Sub = 0x05,
        Print = 0x06,
        Jump = 0x07,
        JumpZero = 0x08,
        JumpNotZero = 0x09
    };

    enum class Register : u8 {
        A = 0,
        B = 1,
        C = 2,
        D = 3
    };

    struct DecodedInstruction {
        Opcode opcode;
        u8 operand1;
        u8 operand2;
    };

    struct ExecutionResult {
        std::optional<u8> output;
        std::optional<u8> jumpTarget;
    };

    inline constexpr u8 InstructionSize = 3;
}