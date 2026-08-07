## CPU Emulator

This Repository is about a simple 8-bit CPU emulator written in C++23. It simulates the fundamental fetch → decode → execute cycle of a processor. It has four registers (A-D), a program counter and a 256-byte adress space implemented as an Array (can store 256x 0-255 numbers; 256 Slots * 1 Byte). 

The assembly language uses its own set of mnemonics (see table below) instead of the usual LOAD/ADD/... names. This is a deliberate split: the C++ code itself (opcode names, function names, ...) stays in plain, professional English, only the little assembly language you write programs in got its own flavor.

## Instruction Set
| Instruction | Pseudocode | Explanation |
| --- | --- | --- |
| LADI a imm | a = imm | Loads an immediate in a Register |
| LADI a [addr] | Reg a = mem[addr] | Loads the value from memory in Register |
| SPEICHERI [addr] a | mem[addr] = a | Writes Register Value in Memory |
| PLUSI a b | a = a+b | Adds two registers |
| MINUSI a b | a = a-b | Subtracts two registers |
| SCHREIBI a | console: a | prints value in console |
| HUEPFI addr | PC = addr | Unconditional jump to a raw byte address |
| HUEPFI0 a addr | if a==0: PC = addr | Jump if register value is zero |
| HUEPFINICHT0 a addr | if a!=0: PC = addr | Jump if register value is not zero |
| STOP | stop | Stop CPU |

Jump targets (`HUEPFI`/`HUEPFI0`/`HUEPFINICHT0`) are raw byte addresses into the same 256-byte memory space that `SPEICHERI`/`LADI [addr]` already use. There are no labels and no two-pass assembly, so target addresses must be computed by hand. Since every instruction is exactly 3 bytes, valid targets are always multiples of 3 relative to the program start.


## 3-byte Format for instructions
| Byte 0 | Byte 1 | Byte 2 |
| --- | --- | --- |
| Opcode | Operand 1 | Operand 2 |


## Register Encoding
- A = 0, B = 1, C = 2, D = 3
- enum class Register: u8

## Example Flow
1. Read .txt File and store instructions in Memory
2. CPU starts at PC=0
3. Each step:
  - Read 3 bytes from Memory
  - print instruction in console
  - execute
  - print cpu status + show ram (only adresses with a stored value)
4. pc += 3
5. Stop

## Example Console View

```text
FETCH [PC=0x06]   PLUSI A B
DECODE Opcode=0x04 Op1=A (0x00) Op2=B (0x01)
EXECUTE A = 10 + 5 = 15
CPU A=15   B=5   C=0   D=0    PC=0x09
RAM (empty)
```
