# SHELLIZ

## Description

Shelliz is simply a small UNIX shell written entirely in C.

Used as a project for the Operating Systems course (Computer Science course at the University of Eastern Piedmont in Italy)

## Tool used

- [MAKE](https://www.gnu.org/software/make/) for compiling multiple sources.
- [GDB](https://www.gnu.org/software/gdb/) as main C debugger.

## Feature

- Background command launch with **&** at the end of the line.
- Enviroment variable in shell prompt.
- All background process are added in BPID's enviroment variable (created at shell'start).
- Shell will ingnore SIGINT signals when a foreground process is running.
- Custom command that not create new process.
- Custom command 'bp' to view BPID enviroment variable.

## How to use

1. Open terminal and clone this repository with ```Git Clone```.
2. Compile with ```Make```.
3. Run the program with ```./smallsh```.
