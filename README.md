## Flow of Kernel Initialization

Listed below is the flow of FreeBSD initialization for an x86 PC. To follow along with the FreeBSD source code
in a logical sequence, read through the source code walkthroughs and consult the documented source code
for further details. 

| Function Executed | FreeBSD Source location | Source Code Walkthrough | Documented Source Code |
|:-----------------:|:---------------------:|:-----------------------:|:----------------------:|
| _start()_ | /boot/i386/boot0/boot0.s | boot.md | boot0.s |
| _main()_ | /boot/i386/boot2/boot1.s | boot.md | boot1.s |
| _init.1()_ | /boot/i386/btx/btx/btx.s | boot.md | btx.s |
| _main()_ | /boot/i386/boot2.c | boot.md | boot2.c |
| _main()_ | /boot/i386/loader/main.c | loader.md | loader.c |
