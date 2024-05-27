
#include <drivers/amd_am79c973.h>
using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;

 


RawDataHandler::RawDataHandler(amd_am79c973* backend)
{
    this->backend = backend;
    backend->SetHandler(this);
}

RawDataHandler::~RawDataHandler()
{
    backend->SetHandler(0);
}
            
bool RawDataHandler::OnRawDataReceived(uint8_t* buffer, uint32_t size)
{
    return false;
}

void RawDataHandler::Send(uint8_t* buffer, uint32_t size)
{
    backend->Send(buffer, size);
}





void printf(char*);
void printfHex(uint8_t);


amd_am79c973::amd_am79c973(PeripheralComponentInterconnectDeviceDescriptor *dev, InterruptManager* interrupts)
:   Driver(),
    /* 
    we pass the interrupt manager and the interupt number reserved, obtained from the device descriptor, which we need to
    increase by the offset that we use for the Hardware Interrupts 
    */
    InterruptHandler(interrupts, dev->interrupt + interrupts->HardwareInterruptOffset()),
    /* portBase is obtained from the PCI */
    MACAddress0Port(dev->portBase),
    MACAddress2Port(dev->portBase + 0x02),
    MACAddress4Port(dev->portBase + 0x04),
    registerDataPort(dev->portBase + 0x10),
    registerAddressPort(dev->portBase + 0x12),
    resetPort(dev->portBase + 0x14),
    busControlRegisterDataPort(dev->portBase + 0x16)
{
    this->handler = 0;
    currentSendBuffer = 0;
    currentRecvBuffer = 0;
    

    uint64_t MAC0 = MACAddress0Port.Read() % 256;
    uint64_t MAC1 = MACAddress0Port.Read() / 256;
    uint64_t MAC2 = MACAddress2Port.Read() % 256;
    uint64_t MAC3 = MACAddress2Port.Read() / 256;
    uint64_t MAC4 = MACAddress4Port.Read() % 256;
    uint64_t MAC5 = MACAddress4Port.Read() / 256;
    
    uint64_t MAC = MAC5 << 40
                 | MAC4 << 32
                 | MAC3 << 24
                 | MAC2 << 16
                 | MAC1 << 8
                 | MAC0;
    
    // 32 bit mode
    registerAddressPort.Write(20);
    /* write 0x102 to the register number 20 */
    busControlRegisterDataPort.Write(0x102);
    
    // we tell that it should not be reset mode
    registerAddressPort.Write(0);
    registerDataPort.Write(0x04);
    
    // initBlock
    initBlock.mode = 0x0000; // promiscuous mode = false
    initBlock.reserved1 = 0;
    /* 8 buffers (2 ^ 3)*/
    initBlock.numSendBuffers = 3;
    initBlock.reserved2 = 0;
    initBlock.numRecvBuffers = 3;
    initBlock.physicalAddress = MAC;
    initBlock.reserved3 = 0;
    initBlock.logicalAddress = 0;
    
    /* we move the pointer 15 bytes to the right and then we kill the last (4?) bits to obtain a 16 bytes aligned address */
    sendBufferDescr = (BufferDescriptor*)((((uint32_t)&sendBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock.sendBufferDescrAddress = (uint32_t)sendBufferDescr;
    recvBufferDescr = (BufferDescriptor*)((((uint32_t)&recvBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock.recvBufferDescrAddress = (uint32_t)recvBufferDescr;
    
    /* above we allocated and move the memory for the descriptors, now we have to set these descriptors */
    for(uint8_t i = 0; i < 8; i++)
    {
        sendBufferDescr[i].address = (((uint32_t)&sendBuffers[i]) + 15 ) & ~(uint32_t)0xF;
        /* in this way we set the length of these descriptors */
        sendBufferDescr[i].flags = 0x7FF
                                 | 0xF000;
        sendBufferDescr[i].flags2 = 0;
        sendBufferDescr[i].avail = 0;
        
        recvBufferDescr[i].address = (((uint32_t)&recvBuffers[i]) + 15 ) & ~(uint32_t)0xF;
        recvBufferDescr[i].flags = 0xF7FF
                                 | 0x80000000;
        recvBufferDescr[i].flags2 = 0;
        sendBufferDescr[i].avail = 0;
    }

    /* after having constructed the initialization block we move it into the device */
    registerAddressPort.Write(1);
    registerDataPort.Write(  (uint32_t)(&initBlock) & 0xFFFF );
    registerAddressPort.Write(2);
    registerDataPort.Write(  ((uint32_t)(&initBlock) >> 16) & 0xFFFF );
    
}

amd_am79c973::~amd_am79c973()
{
}
            
void amd_am79c973::Activate()
{
    /* this enables the interrupts */
    registerAddressPort.Write(0);
    registerDataPort.Write(0x41);

    registerAddressPort.Write(4);
    uint32_t temp = registerDataPort.Read();
    registerAddressPort.Write(4);
    registerDataPort.Write(temp | 0xC00);
    
    registerAddressPort.Write(0);
    registerDataPort.Write(0x42);
}

int amd_am79c973::Reset()
{
    resetPort.Read();
    resetPort.Write(0);
    /* return 10 means that we want to wait for 10 milliseconds */
    return 10;
}


uint32_t amd_am79c973::HandleInterrupt(common::uint32_t esp)
{
    /* if there is data we need to fetch (read) it */
    registerAddressPort.Write(0);
    uint32_t temp = registerDataPort.Read();
    
    if((temp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n");
    if((temp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
    /* we get too much data, faster than we can handle it */
    if((temp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n");
    if((temp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");

    /* data received successfully */
    if((temp & 0x0400) == 0x0400) Receive();
    /* if we receive 200 than it has successfully sent data */
    if((temp & 0x0200) == 0x0200) printf(" SENT");
                               
    // acknowledge
    registerAddressPort.Write(0);
    registerDataPort.Write(temp);
    
    if((temp & 0x0100) == 0x0100) printf("AMD am79c973 INIT DONE\n");
    
    return esp;
}

       
void amd_am79c973::Send(uint8_t* buffer, int size)
{
    /* where we want to send the data */
    int sendDescriptor = currentSendBuffer;
    /* move to the next send buffer in cyclic way so to send data from different tasks in parallel */
    currentSendBuffer = (currentSendBuffer + 1) % 8;
    
    /* if we try to send more than 1518 bytes of data, we discard data after 1518 byte */
    if(size > 1518)
        size = 1518;
    
    /* data copied in the send buffer that we have selected */
    for(uint8_t *src = buffer + size -1, /* pointer set to the end of the data that we want to send */
                *dst = (uint8_t*)(sendBufferDescr[sendDescriptor].address + size -1);
                src >= buffer; src--, dst--)
        *dst = *src; /* copy data from source to destination buffer */
        
    printf("\nSEND: ");
    for(int i = 14+20; i < (size>64?64:size); i++)
    {
        printfHex(buffer[i]);
        printf(" ");
    }
    
    /* we cannot write anything in the buffer until it becomes available again */
    sendBufferDescr[sendDescriptor].avail = 0;

    /* see documentation about the AMD chip to understand what these flags mean */
    sendBufferDescr[sendDescriptor].flags2 = 0;
    sendBufferDescr[sendDescriptor].flags = 0x8300F000
                                          | ((uint16_t)((-size) & 0xFFF));

    /* send the data in the 0 register */                                      
    registerAddressPort.Write(0);
    /* this is the send command */
    registerDataPort.Write(0x48);
}

void amd_am79c973::Receive()
{
    printf("\nRECV: ");
    
    /* iterate through the receive buffers as long as we have received buffers that have data */
    for(; (recvBufferDescr[currentRecvBuffer].flags & 0x80000000) == 0; /* if first flag is 0 the buffer is empty */
        currentRecvBuffer = (currentRecvBuffer + 1) % 8)
    {
        if(!(recvBufferDescr[currentRecvBuffer].flags & 0x40000000) /* checks the error bit */
         && (recvBufferDescr[currentRecvBuffer].flags & 0x03000000) == 0x03000000) /* check start of packet (STP) and end of packet (ETP) bits */
        
        {
            /* read the size from the receive buffer */
            uint32_t size = recvBufferDescr[currentRecvBuffer].flags & 0xFFF;
            /* if size is > 64 (this is an ethernet frame) */
            if(size > 64) // remove checksum
                /* remove last 4 bytes which are the checksum */
                size -= 4;
            
            uint8_t* buffer = (uint8_t*)(recvBufferDescr[currentRecvBuffer].address);

            /* iterate over the data and print what we have received */
            for(int i = 14+20; i < (size>64?64:size); i++)
            {
                printfHex(buffer[i]);
                printf(" ");
            }
 
            if(handler != 0)
                if(handler->OnRawDataReceived(buffer, size))
                    Send(buffer, size);
        }
        
        /* we have finished handling this, you can have it (the buffer) back now */
        recvBufferDescr[currentRecvBuffer].flags2 = 0;
        recvBufferDescr[currentRecvBuffer].flags = 0x8000F7FF;
    }
}

void amd_am79c973::SetHandler(RawDataHandler* handler)
{
    this->handler = handler;
}

uint64_t amd_am79c973::GetMACAddress()
{
    return initBlock.physicalAddress;
}

void amd_am79c973::SetIPAddress(uint32_t ip)
{
    initBlock.logicalAddress = ip;
}

uint32_t amd_am79c973::GetIPAddress()
{
    return initBlock.logicalAddress;
}