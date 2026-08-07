#include "assembler.hpp"
#include "instruction.hpp"
#include <fstream>
#include <limits>
#include <sstream>

using namespace JanSordid::Core;

namespace MiniCPU {

    [[nodiscard]] static std::optional<Register> parseRegister(StringView token) noexcept {
        if(token == "A") return Register::A;
        if(token == "B") return Register::B;
        if(token == "C") return Register::C;
        if(token == "D") return Register::D;
        return std::nullopt;
    }

    //liest Zahl als Text und macht daraus eine Zahl
    [[nodiscard]] static std::optional<u8> parseImmediate(StringView token) noexcept {
        if(token.empty()) return std::nullopt;
        int val = 0;
        for(char c : token) {
            if(c < '0' || c > '9') return std::nullopt;
            val = val * 10 + (c - '0');
            if(val > std::numeric_limits<u8>::max()) return std::nullopt;
        }
        return static_cast<u8>(val);
    }

    //trennt [ / ] weg
    [[nodiscard]] static std::optional<u8> parseMemAddr(StringView token) noexcept {
        if(token.size() < 3) return std::nullopt;
        if(token.front() != '[' || token.back() != ']') return std::nullopt;
        return parseImmediate(token.substr(1, token.size() - 2));
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseStop(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 1) return std::nullopt;
        return {{ static_cast<u8>(Opcode::Stop), 0, 0 }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseLoad(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 3) return std::nullopt;
        const auto reg = parseRegister(tokens[1]);
        if(!reg) return std::nullopt;
        const u8 regByte = static_cast<u8>(*reg);

        if(const auto imm = parseImmediate(tokens[2]))
            return {{ static_cast<u8>(Opcode::LoadImm), regByte, *imm }};
        if(const auto addr = parseMemAddr(tokens[2]))
            return {{ static_cast<u8>(Opcode::LoadMem), regByte, *addr }};
        return std::nullopt;
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseStore(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 3) return std::nullopt;
        const auto addr = parseMemAddr(tokens[1]);
        const auto reg  = parseRegister(tokens[2]);
        if(!addr || !reg) return std::nullopt;
        return {{ static_cast<u8>(Opcode::Store), *addr, static_cast<u8>(*reg) }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseAdd(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 3) return std::nullopt;
        const auto r1 = parseRegister(tokens[1]);
        const auto r2 = parseRegister(tokens[2]);
        if(!r1 || !r2) return std::nullopt;
        return {{ static_cast<u8>(Opcode::Add), static_cast<u8>(*r1), static_cast<u8>(*r2) }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseSub(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 3) return std::nullopt;
        const auto r1 = parseRegister(tokens[1]);
        const auto r2 = parseRegister(tokens[2]);
        if(!r1 || !r2) return std::nullopt;
        return {{ static_cast<u8>(Opcode::Sub), static_cast<u8>(*r1), static_cast<u8>(*r2) }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parsePrint(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 2) return std::nullopt;
        const auto reg = parseRegister(tokens[1]);
        if(!reg) return std::nullopt;
        return {{ static_cast<u8>(Opcode::Print), static_cast<u8>(*reg), 0 }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseJump(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 2) return std::nullopt;
        const auto addr = parseImmediate(tokens[1]);
        if(!addr) return std::nullopt;
        return {{ static_cast<u8>(Opcode::Jump), 0, *addr }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseJumpZero(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 3) return std::nullopt;
        const auto reg = parseRegister(tokens[1]);
        const auto addr = parseImmediate(tokens[2]);
        if(!reg || !addr) return std::nullopt;
        return {{ static_cast<u8>(Opcode::JumpZero), static_cast<u8>(*reg), *addr }};
    }

    [[nodiscard]] static std::optional<std::array<u8, 3>> parseJumpNotZero(const DynArray<String>& tokens) noexcept {
        if(tokens.size() != 3) return std::nullopt;
        const auto reg = parseRegister(tokens[1]);
        const auto addr = parseImmediate(tokens[2]);
        if(!reg || !addr) return std::nullopt;
        return {{ static_cast<u8>(Opcode::JumpNotZero), static_cast<u8>(*reg), *addr }};
    }

    //verteilt an die passende parseXyz()-Funktion je nach Opcode-Token; std::nullopt bei leerer/unbekannter Zeile
    [[nodiscard]] static std::optional<std::array<u8, 3>> parseLine(const DynArray<String>& tokens) noexcept {
        if(tokens.empty()) return std::nullopt;
        const StringView op = tokens[0];

        if(op == "STOP")         return parseStop(tokens);
        if(op == "LADI")         return parseLoad(tokens);
        if(op == "SPEICHERI")    return parseStore(tokens);
        if(op == "PLUSI")        return parseAdd(tokens);
        if(op == "MINUSI")       return parseSub(tokens);
        if(op == "SCHREIBI")     return parsePrint(tokens);
        if(op == "HUEPFI")       return parseJump(tokens);
        if(op == "HUEPFI0")      return parseJumpZero(tokens);
        if(op == "HUEPFINICHT0") return parseJumpNotZero(tokens);
        return std::nullopt;
    }

    std::optional<DynArray<u8>> assemble(const String& filepath) {
        std::ifstream file(filepath);
        if(!file.is_open()) {
            print("Assembler Fehler: Kann '{}' nicht oeffnen\n", filepath);
            return std::nullopt;
        }

        DynArray<u8> result;
        String line;
        usize lineNum = 0;

        while(std::getline(file, line)) {
            ++lineNum;
            //Kommentarzeilen erkennen: '#'
            const usize firstNonSpace = line.find_first_not_of(" \t");
            if(firstNonSpace == String::npos || line[firstNonSpace] == '#') continue;

            DynArray<String> tokens;
            std::istringstream iss{line};
            String token;
            while(iss >> token)
                tokens.push_back(std::move(token));

            if(tokens.empty()) continue;

            const auto bytes = parseLine(tokens);
            if(!bytes) {
                print("Assembler Fehler: Zeile {}: '{}'\n", lineNum, line);
                return std::nullopt;
            }

            result.push_back((*bytes)[0]);
            result.push_back((*bytes)[1]);
            result.push_back((*bytes)[2]);
        }
        return result;
    }
}