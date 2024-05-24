 
#include <memorymanagement.h>

using namespace myos;
using namespace myos::common;


MemoryManager* MemoryManager::activeMemoryManager = 0;
        
MemoryManager::MemoryManager(size_t start, size_t size)
{
    activeMemoryManager = this;
    
    /* without this check it could happen that we would write outside the area we are allowed to write */
    if(size < sizeof(MemoryChunk))
    {
        first = 0;
    }
    else
    {
        /* where we put the first memory chunk */
        first = (MemoryChunk*)start;
        
        first -> allocated = false;
        first -> prev = 0;
        first -> next = 0;
        first -> size = size - sizeof(MemoryChunk);
    }
}

MemoryManager::~MemoryManager()
{
    if(activeMemoryManager == this)
        activeMemoryManager = 0;
}
        
void* MemoryManager::malloc(size_t size)
{
    MemoryChunk *result = 0;
    
    for(MemoryChunk* chunk = first; chunk != 0 && result == 0; chunk = chunk->next)
        if(chunk->size > size && !chunk->allocated)
            /* we set the result pointer to the chunk */
            result = chunk;
        
    if(result == 0)
        return 0;
    
    /* the result is set to something */
    if(result->size >= size + sizeof(MemoryChunk) + 1)
    {
        /* temp is the chunk of free space not allocated that follows the chunk we allocate now (result) */
        /* we make a new chunk and we put that to the position of the result (pointer) + the sizeof MemoryChunk + the size requested */
        MemoryChunk* temp = (MemoryChunk*)((size_t)result + sizeof(MemoryChunk) + size);
        
        temp->allocated = false;
        /* the new chunk that we have cut off is equal to the old chunk minus the new chunk that we have requested */
        temp->size = result->size - size - sizeof(MemoryChunk);
        temp->prev = result;
        temp->next = result->next;
        if(temp->next != 0)
            temp->next->prev = temp;
        
        result->size = size;
        result->next = temp;
    }
    
    /* if the chunk is too small for the requested size we mark it as allocated */
    result->allocated = true;
    return (void*)(((size_t)result) + sizeof(MemoryChunk));
}

void MemoryManager::free(void* ptr)
{
    MemoryChunk* chunk = (MemoryChunk*)((size_t)ptr - sizeof(MemoryChunk));
    
    chunk -> allocated = false;
    
    /* we merge the consecutive free chunks (if we have any) in particular we merge a chunk with the previous one */
    if(chunk->prev != 0 && !chunk->prev->allocated)
    {
        /* we set the next pointer of the previous one to the next of the chunk we want to merge with the previous one (current) */
        chunk->prev->next = chunk->next;
        chunk->prev->size += chunk->size + sizeof(MemoryChunk);
        if(chunk->next != 0)
            chunk->next->prev = chunk->prev;
        
        chunk = chunk->prev;
    }
    
    if(chunk->next != 0 && !chunk->next->allocated)
    {
        chunk->size += chunk->next->size + sizeof(MemoryChunk);
        chunk->next = chunk->next->next;
        if(chunk->next != 0)
            chunk->next->prev = chunk;
    }
    
}




void* operator new(unsigned size)
{
    if(myos::MemoryManager::activeMemoryManager == 0)
        return 0;
    return myos::MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new[](unsigned size)
{
    if(myos::MemoryManager::activeMemoryManager == 0)
        return 0;
    return myos::MemoryManager::activeMemoryManager->malloc(size);
}

/*
If we don't find any place to put the requested memory (object) the loop terminates and we return 0 (NULL).
Malloc is supposed to throw an exception if it doesn0t find anything. But we don't have exception handling and we return 0.
If malloc returns 0, then the new, will say: ok i got something, use it. Giving that i had 0, i would dereference 0 (NULL) and i should
have a NULL pointer exception. But this happens in user mode, in kernel mode i can obtain the memory location 0 and try to dereference it
so in kernel mode it is allowed. So in order to have a new to use in user mode, we implement other versions of new called 'replacement new'.
They just return the pointer, so this is used to call a constructor explicitly on memory that we have allocated already.
*/
void* operator new(unsigned size, void* ptr)
{
    return ptr;
}

void* operator new[](unsigned size, void* ptr)
{
    return ptr;
}

void operator delete(void* ptr)
{
    if(myos::MemoryManager::activeMemoryManager != 0)
        myos::MemoryManager::activeMemoryManager->free(ptr);
}

void operator delete[](void* ptr)
{
    if(myos::MemoryManager::activeMemoryManager != 0)
        myos::MemoryManager::activeMemoryManager->free(ptr);
}