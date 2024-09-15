# Chip8
An implementation of a Chip8 emulator

This project served as a way to play around and explore building emulators. This project borrowed heavily from Austin Morlan blog post https://austinmorlan.com/posts/chip8_emulator/ and Cowgod's chip 8 technical manual http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0.

# What is Chip-8?

Chip-8 is a vitural machine from the 90s designed to play .ch8 rom games. Due to being a virutal machine and not a real world product and whoses instruction set consits of 34 instructions, it serves as a simple and highly portable virutal console.

# Compiling

With MSYS2 compile the code with this command on windows

 g++ -L.\mingw -I..\include -std=c++20 -Wall -DPLOT main.cpp -lmingw32 -lSDL2main -lSDL2
