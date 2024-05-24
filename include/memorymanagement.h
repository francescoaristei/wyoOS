 
#ifndef __MYOS__MEMORYMANAGEMENT_H
#define __MYOS__MEMORYMANAGEMENT_H

#include <common/types.h>


namespace myos
{
    
    struct MemoryChunk
    {
        MemoryChunk *next;
        MemoryChunk *prev;
        bool allocated;
        common::size_t size;
    };
    
    
    class MemoryManager
    {
        
    protected:
        /* pointer to the first chunk */
        MemoryChunk* first;
    public:
        
        static MemoryManager *activeMemoryManager;
        
        MemoryManager(common::size_t first, common::size_t size);
        ~MemoryManager();
        
        void* malloc(common::size_t size);
        void free(void* ptr);
    };
}

/* we introduce the new operator to allocate memory (the new operator from C++). We need to define it ourself if we manage the memory */
void* operator new(unsigned size);
void* operator new[](unsigned size);

// placement new
void* operator new(unsigned size, void* ptr);
void* operator new[](unsigned size, void* ptr);

/* same goes for the delete */
void operator delete(void* ptr);
void operator delete[](void* ptr);


#endif