
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
/* add the offset so that we don't have to add it in the kernel.cpp */
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}


void printf(char*);

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    /* the ESP points to the start of this CPU state struct */
    CPUState* cpu = (CPUState*)esp;
    

    /* in case 4 we would do printf */
    switch(cpu->eax)
    {
        /*
        The POSIX standard define which values of the EAX register means which interrupt number 
        so extending it is possible to implement the backend for opening file, etc. Actually
        is possible to build some kind of glibc, a library that we can statically link to the binary
        that allows to perform system calls. Which means that if we have all the methods of the glibc we can
        execute all the c programs that comply to the standard with our OS.
        */
        case 4:
            printf((char*)cpu->ebx);
            break;
            
        default:
            break;
    }

    
    return esp;
}

