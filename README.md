# DISCLAIMER
***Before using this, you ABSOLUTELY need to have some experience with operating system creation.
I wont waste my time answering questions like "How to i make better gui", "How to make it support windows executables" and etc.
If you dont have required knowledge (https://wiki.osdev.org/Required_Knowledge) then don't use this repository. Lets continue.***

# MinautoryOS V1.0
MinautoryOS is a lightweight skeleton of a operating system. It has a cli with a couple of commands (accessible through ***Debug*** option), a dos-like gui, and a working fault handler (you can test it in cli using ```crashtest <num>```. Everything else is just a placeholder. You can download this repository and make a fully working OS out of it.
# Building
Before building, ensure you have following dependencies:
```
GRUB (sudo apt install grub)
QEMU (sudo apt install qemu-system)
GCC-Crosscompiler (https://wiki.osdev.org/GCC_Cross-Compiler)
```
Then, modify the help.sh file and modify these lines:
```
/home/vasil/opt/cross/bin/i686-elf-as boot.s -o boot.o
/home/vasil/opt/cross/bin/i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
/home/vasil/opt/cross/bin/i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
```
to:
```
/home/<username>/opt/cross/bin/i686-elf-as boot.s -o boot.o
/home/<username>/opt/cross/bin/i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
/home/<username>/opt/cross/bin/i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
```
Now, just run the help.sh and it will create an .iso file and run it in qemu. You can also run it in any other emulator like VirtualBox.
<img width="748" height="493" alt="image" src="https://github.com/user-attachments/assets/07a6d1b4-7c1a-46f2-8210-3c6b103c3992" />
<img width="748" height="493" alt="image" src="https://github.com/user-attachments/assets/3784a716-8997-4647-97b1-215181e9a834" />
<img width="748" height="493" alt="image" src="https://github.com/user-attachments/assets/ad8da724-42b7-4727-b267-f4035fd8a022" />

# CLI commands
A list of current commands in the CLI mode:
```
sysinfo - Gives information about your cpu
```

***Happy Coding!***
