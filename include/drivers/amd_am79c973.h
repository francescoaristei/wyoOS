 
#ifndef __MYOS__DRIVERS__AMD_AM79C973_H
#define __MYOS__DRIVERS__AMD_AM79C973_H


#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/pci.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>


namespace myos
{
    namespace drivers
    {
        
        class amd_am79c973;
        
        /* handler of the data received as ethernet frames */
        class RawDataHandler
        {
        protected:
            /* the handler needs to know where it is receiving data from */
            amd_am79c973* backend;
        public:
            RawDataHandler(amd_am79c973* backend);
            ~RawDataHandler();
            
            virtual bool OnRawDataReceived(common::uint8_t* buffer, common::uint32_t size);
            void Send(common::uint8_t* buffer, common::uint32_t size);
        };
        
        /* this driver will be an interrupt handler and it will have different ports */
        class amd_am79c973 : public Driver, public hardwarecommunication::InterruptHandler
        {
            struct InitializationBlock
            {
                /* needed for promiscuous mode */
                common::uint16_t mode;
                unsigned reserved1 : 4;
                /* number of received and send buffers, expressed in bits (if we write 3 means we have 2^3=8 buffers) */
                unsigned numSendBuffers : 4;
                unsigned reserved2 : 4;
                unsigned numRecvBuffers : 4;
                /* MAC address */
                common::uint64_t physicalAddress : 48;
                common::uint16_t reserved3;
                /* IP address (?) */
                common::uint64_t logicalAddress;
                common::uint32_t recvBufferDescrAddress;
                common::uint32_t sendBufferDescrAddress;
            } __attribute__((packed));
            
            
            struct BufferDescriptor
            {
                common::uint32_t address;
                common::uint32_t flags;
                common::uint32_t flags2;
                common::uint32_t avail;
            } __attribute__((packed));
            
            /* 3 ports for reading the MAC address, each of them gives 2 bytes of the MAC address */
            hardwarecommunication::Port16Bit MACAddress0Port;
            hardwarecommunication::Port16Bit MACAddress2Port;
            hardwarecommunication::Port16Bit MACAddress4Port;
            hardwarecommunication::Port16Bit registerDataPort;
            hardwarecommunication::Port16Bit registerAddressPort;
            hardwarecommunication::Port16Bit resetPort;
            hardwarecommunication::Port16Bit busControlRegisterDataPort;
            
            /* struct needed to hold a pointer to the array of BufferDescriptors, which hold pointers to the buffers */
            InitializationBlock initBlock;
            
            
            BufferDescriptor* sendBufferDescr;
            /* memory needed for the descriptors */
            common::uint8_t sendBufferDescrMemory[2048+15];
            common::uint8_t sendBuffers[2*1024+15][8];
            /* number to tell which of the send buffers are currently active */
            common::uint8_t currentSendBuffer;
            
            /* we put the pointer to the buffer descriptor in the recvBufferDescrMemory memory we allocated */
            BufferDescriptor* recvBufferDescr;
            common::uint8_t recvBufferDescrMemory[2048+15];
            common::uint8_t recvBuffers[2*1024+15][8];
            /* number to tell which of the receive buffers are currently active */
            common::uint8_t currentRecvBuffer;
            
            
            RawDataHandler* handler;
            
        public:
            amd_am79c973(myos::hardwarecommunication::PeripheralComponentInterconnectDeviceDescriptor *dev,
                         myos::hardwarecommunication::InterruptManager* interrupts);
            ~amd_am79c973();
            
            /* inherited and overridden from Driver */
            void Activate();
            int Reset();
            /* inherited and overridden from InterruptHandler */
            common::uint32_t HandleInterrupt(common::uint32_t esp);
            
            void Send(common::uint8_t* buffer, int count);
            void Receive();
            
            void SetHandler(RawDataHandler* handler);
            common::uint64_t GetMACAddress();
            /* set the IP address manually not with an DHCP server. */
            void SetIPAddress(common::uint32_t);
            common::uint32_t GetIPAddress();
        };
        
        
        
    }
}

#endif
