# TD4M and TD4 CPU Emulators
## TD4M CPU Emulator
This project features the TD4M CPU emulator, implemented in C++. It recreates the pseudo architecture of the TD4M processor, a conceptual evolution inspired by the original TD4 CPU. The emulator simulates the core components and behavior of the TD4M, providing an interactive and faithful environment for experimenting with this hypothetical architecture.

- You can load your TD4M machine code instructions into the emulator’s ROM in hex format.
- The emulator executes these instructions step-by-step.
- The currently executing line of code is highlighted in red for easy tracking.
-When the CPU requires input during execution, the input field is highlighted in green.
- Includes a debugging console where you can set breakpoints, step through code, inspect and print registers, and monitor execution state interactively.

This emulator serves as a platform for exploring advanced CPU design concepts beyond the original TD4, enabling experimentation with extended instruction sets and architectural features.
This enhanced debugging capability makes the TD4M emulator an excellent tool for in-depth CPU design experimentation and learning.


## TD4 CPU Emulator
This project is a TD4 emulator implemented in C. It faithfully recreates the architecture of the real TD4 computing system, providing an accurate simulation of its core components including selectors, the Arithmetic Logic Unit (ALU), and the data bus.
You can load into it's ROM your instructions in hex format, then TD4  will execute them. 

- Emulates the TD4 architecture from the ground up.
- Implements the selectors to manage data flow and control signals.
- Simulates the ALU operations with full functionality.
- Models the data bus behavior to coordinate information transfer.
- Designed for clarity and fidelity to the original hardware.

Line of code that is executed rigth now will be highlited by red. When processor will need an input, input space will be highlited by green.

## Assembly compiler
Provides an assembly compiler to write and compile assembly code directly for TD4/TD4M, simplifying program development.

## Some programs for TD4:
| ROM in hex  | meaning |
|----------------------------------|----------------|
| 20000fe75ff2005f90f0000000000000 | reverse bits   |
| 20600fef51f200000000000000000090 | sum of A and B |
| 33054f9ff00000000000000000000000 | 8 to 6         |


## Principle scheme of TD4
| CPU | ROM |
|---------|----------|
|<img src="pic/cpu.png" alt="CPU itself" > |<img src="pic/input.png" alt="CPU itself" >|

## Offisial TD4 opcode
| Mnemo | Opcode |
|---------|----------|
|ADD A,Im | 0000 xxxx|
|ADD B,Im | 0101 xxxx|
|MOV A,Im | 0011 xxxx|
|MOV B,Im | 0111 xxxx|
|MOV A,B  | 0001 0000|
|MOV B,A  | 0100 0000|
|JMP Im   | 1111 xxxx|
|JNC Im   | 1110 xxxx|
|IN A     | 0010 0000|
|IN B     | 0110 0000|
|OUT B    | 1001 0000|
|OUT IM   | 1011 xxxx|

where xxxx - some data.


## Unoffisial TD4 opcode 
1000 - add a number to B register, write into OUT.\
1010 - add a number to ZERO, write into OUT.\
1100 - add a number to B register, write into PC if there is no CF.\
1101 - add a number to B register, write into PC.
