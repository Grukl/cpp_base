#include "cpu.hpp"

using namespace JanSordid::Core;

namespace MiniCPU {

    /*--- CPU-Kern ---*/

    u8 CPU::registerValue(Register reg) const noexcept {
        const auto index = static_cast<u8>(reg);
        Assert(index < RegisterCount);
        return _registers[index];
    }

    void CPU::setRegister(Register reg, u8 value) noexcept {
        const auto index = static_cast<u8>(reg);
        Assert(index < RegisterCount);
        _registers[index] = value;
    }

    DecodedInstruction CPU::fetch(const Memory& memory) const noexcept {
        const u8 opcodeByte = memory.read(_pc);
        const u8 operand1 = memory.read(static_cast<u8>(_pc + 1));
        const u8 operand2 = memory.read(static_cast<u8>(_pc + 2));
        return { static_cast<Opcode>(opcodeByte), operand1, operand2 };
    }

    ExecutionResult CPU::execute(const DecodedInstruction& instruction, Memory& memory) noexcept {
        switch(instruction.opcode) {
            case Opcode::LoadImm:
                setRegister(static_cast<Register>(instruction.operand1), instruction.operand2);
                return {};
            case Opcode::LoadMem:
                setRegister(static_cast<Register>(instruction.operand1), memory.read(instruction.operand2));
                return {};
            case Opcode::Store:
                memory.write(instruction.operand1, registerValue(static_cast<Register>(instruction.operand2)));
                return {};
            case Opcode::Add: {
                const auto r1 = static_cast<Register>(instruction.operand1);
                const auto r2 = static_cast<Register>(instruction.operand2);
                setRegister(r1, static_cast<u8>(registerValue(r1) + registerValue(r2)));
                return {};
            }
            case Opcode::Sub: {
                const auto r1 = static_cast<Register>(instruction.operand1);
                const auto r2 = static_cast<Register>(instruction.operand2);
                setRegister(r1, static_cast<u8>(registerValue(r1) - registerValue(r2)));
                return {};
            }
            case Opcode::Print:
                return { registerValue(static_cast<Register>(instruction.operand1)), std::nullopt };
            case Opcode::Jump:
                return { std::nullopt, instruction.operand2 };
            case Opcode::JumpZero:
                if(registerValue(static_cast<Register>(instruction.operand1)) == 0)
                    return { std::nullopt, instruction.operand2 };
                return {};
            case Opcode::JumpNotZero:
                if(registerValue(static_cast<Register>(instruction.operand1)) != 0)
                    return { std::nullopt, instruction.operand2 };
                return {};
            case Opcode::Stop:
                Assert(false);
                return {};
        }
        Assert(false);
        return {};
    }

    void CPU::run(Memory& memory) noexcept {
        u32 step = 1;
        while(true) {
            const u8 pcAtFetch = _pc;
            const DecodedInstruction instruction = fetch(memory);
            const bool isStop = instruction.opcode == Opcode::Stop;

            ExecutionResult result;
            if(!isStop) {
                result = execute(instruction, memory);
                _pc = result.jumpTarget.value_or(static_cast<u8>(_pc + InstructionSize));
            }

            printStep(step, pcAtFetch, instruction, memory, result.output, isStop);

            if(isStop)
                break;

            ++step;
        }
        print("\n");
    }


    /*--- Anzeige/Konsolen-Ausgabe ---*/

    [[nodiscard]] static char registerLetter(Register reg) noexcept {
        return static_cast<char>('A' + static_cast<u8>(reg));
    }

    [[nodiscard]] static String disassemble(const DecodedInstruction& instruction) noexcept {
        switch(instruction.opcode) {
            case Opcode::Stop:
                return "STOP";
            case Opcode::LoadImm:
                return format("LADI {} {}", registerLetter(static_cast<Register>(instruction.operand1)), instruction.operand2);
            case Opcode::LoadMem:
                return format("LADI {} [{}]", registerLetter(static_cast<Register>(instruction.operand1)), instruction.operand2);
            case Opcode::Store:
                return format("SPEICHERI [{}] {}", instruction.operand1, registerLetter(static_cast<Register>(instruction.operand2)));
            case Opcode::Add:
                return format("PLUSI {} {}", registerLetter(static_cast<Register>(instruction.operand1)), registerLetter(static_cast<Register>(instruction.operand2)));
            case Opcode::Sub:
                return format("MINUSI {} {}", registerLetter(static_cast<Register>(instruction.operand1)), registerLetter(static_cast<Register>(instruction.operand2)));
            case Opcode::Print:
                return format("SCHREIBI {}", registerLetter(static_cast<Register>(instruction.operand1)));
            case Opcode::Jump:
                return format("HUEPFI {}", instruction.operand2);
            case Opcode::JumpZero:
                return format("HUEPFI0 {} {}", registerLetter(static_cast<Register>(instruction.operand1)), instruction.operand2);
            case Opcode::JumpNotZero:
                return format("HUEPFINICHT0 {} {}", registerLetter(static_cast<Register>(instruction.operand1)), instruction.operand2);
        }
        Assert(false);
        return "UNKNOWN";
    }

    static constexpr usize BytesPerRow = 16;
    static_assert(Memory::Size % BytesPerRow == 0, "Memory::Size muss ein Vielfaches von BytesPerRow sein");

    DynArray<String> CPU::buildInstructionLines(u32 step, u8 pcAtFetch, const DecodedInstruction& instruction,
            std::optional<u8> output, bool isStop) const noexcept {
        DynArray<String> lines;
        lines.push_back(format("STEP {:02}", step));
        lines.push_back(format("FETCH  [PC=0x{:02X}] {}", pcAtFetch, disassemble(instruction)));
        lines.push_back(format("DECODE Opcode=0x{:02X} Op1=0x{:02X} Op2=0x{:02X}",
            static_cast<u8>(instruction.opcode), instruction.operand1, instruction.operand2));
        if(output.has_value())
            lines.push_back(format("OUTPUT {}", *output));
        if(isStop)
            lines.push_back("STOP - Programm beendet");
        return lines;
    }

    DynArray<String> CPU::buildRegisterLines() const noexcept {
        DynArray<String> lines;
        lines.push_back("REGISTERS");
        for(u8 i = 0; i < RegisterCount; ++i) {
            const auto reg = static_cast<Register>(i);
            const u8 value = registerValue(reg);
            lines.push_back(format("  {} = {:<3} (0x{:02X})", registerLetter(reg), value, value));
        }
        lines.push_back(format("PC = 0x{:02X}", _pc));
        return lines;
    }

    DynArray<String> CPU::buildRamLines(const Memory& memory, u8 highlightStart) const noexcept {
        const u8 highlightMid = static_cast<u8>(highlightStart + 1);
        const u8 highlightEnd = static_cast<u8>(highlightStart + 2);

        usize highestNonZeroRow = 0;
        for(usize addr = 0; addr < Memory::Size; ++addr)
            if(memory.read(static_cast<u8>(addr)) != 0)
                highestNonZeroRow = addr / BytesPerRow;
        const usize highlightRow = highlightStart / BytesPerRow;
        const usize rowCount = max(highestNonZeroRow, highlightRow) + 1;
        Assert(rowCount <= Memory::Size / BytesPerRow);

        DynArray<String> lines;
        lines.push_back("RAM");

        String header = "      ";
        for(usize col = 0; col < BytesPerRow; ++col) {
            if(col > 0)
                header += "|";
            header += format(" {:02X} ", col);
        }
        lines.push_back(header);

        String separator = "------";
        for(usize col = 0; col < BytesPerRow; ++col) {
            if(col > 0)
                separator += "+";
            separator += "----";
        }
        lines.push_back(separator);

        for(usize row = 0; row < rowCount; ++row) {
            String line = format("0x{:02X}  ", row * BytesPerRow);
            for(usize col = 0; col < BytesPerRow; ++col) {
                const u8 addr = static_cast<u8>(row * BytesPerRow + col);
                const u8 value = memory.read(addr);
                const bool isCurrentInstruction = addr == highlightStart || addr == highlightMid || addr == highlightEnd;

                if(col > 0)
                    line += "|";

                if(isCurrentInstruction)
                    line += format("[{:02X}]", value);
                else if(value == 0)
                    line += "  . ";
                else
                    line += format(" {:02X} ", value);
            }
            lines.push_back(line);
        }

        return lines;
    }

    void CPU::printSideBySide(const DynArray<String>& leftLines, const DynArray<String>& rightLines) noexcept {
        Assert(!leftLines.empty());
        Assert(!rightLines.empty());

        usize leftWidth = 0;
        for(const auto& line : leftLines)
            leftWidth = max(leftWidth, line.size());

        usize rightWidth = 0;
        for(const auto& line : rightLines)
            rightWidth = max(rightWidth, line.size());

        const String border = format("+{}+{}+\n", String(leftWidth + 2, '-'), String(rightWidth + 2, '-'));

        print("{}", border);

        const usize rowCount = max(leftLines.size(), rightLines.size());
        for(usize row = 0; row < rowCount; ++row) {
            const String left  = row < leftLines.size()  ? leftLines[row]  : String{};
            const String right = row < rightLines.size() ? rightLines[row] : String{};
            print("| {:<{}} | {:<{}} |\n", left, leftWidth, right, rightWidth);
        }

        print("{}", border);
    }

    void CPU::printStep(u32 step, u8 pcAtFetch, const DecodedInstruction& instruction, const Memory& memory,
            std::optional<u8> output, bool isStop) const noexcept {
        DynArray<String> leftLines = buildInstructionLines(step, pcAtFetch, instruction, output, isStop);
        leftLines.push_back("");
        const DynArray<String> registerLines = buildRegisterLines();
        leftLines.insert(leftLines.end(), registerLines.begin(), registerLines.end());

        const DynArray<String> rightLines = buildRamLines(memory, pcAtFetch);

        print("\n");
        printSideBySide(leftLines, rightLines);
    }
}