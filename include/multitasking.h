 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{
    
    struct CPUState
    {
        /* all the registers making the CPU state */
        /* pushed by the interruptstubs */
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        /* pushed by the processor */
        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    
    
    class Task
    {
    friend class TaskManager;
    private:
        /* we allocate some stack */
        common::uint8_t stack[4096]; // 4 KiB
        /* 
        The pointer to the top of the task stack is a pointer to the top of the CPUState object which contains the state of the CPU 
        that we write in the task stack 
        */
        CPUState* cpustate;
    public:
        /* entrypoint is a pointer to the function that needs to be executed */
        Task(GlobalDescriptorTable *gdt, void entrypoint());
        ~Task();
    };
    
    
    class TaskManager
    {
    private:
        Task* tasks[256];
        int numTasks;
        /* here we will save the task that we executed before changing to a new task, otherwise we don't know how to get back to it */
        int currentTask;
    public:
        TaskManager();
        ~TaskManager();
        bool AddTask(Task* task);
        /* method with the scheduling algorithm (round robin) */
        CPUState* Schedule(CPUState* cpustate);
    };
    
    
    
}


#endif