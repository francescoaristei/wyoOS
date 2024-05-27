 
#ifndef __MYOS__DRIVERS__ATA_H
#define __MYOS__DRIVERS__ATA_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>

namespace myos
{
    namespace drivers
    {
        
        class AdvancedTechnologyAttachment
        {
        protected:
            bool master;
            /* nine ports needed to communicate with the hard drive */
            hardwarecommunication::Port16Bit dataPort; /* to pull the data we want to write and take the data we want to read */
            hardwarecommunication::Port8Bit errorPort;
            hardwarecommunication::Port8Bit sectorCountPort;
            hardwarecommunication::Port8Bit lbaLowPort; /* address of the sector we want to read */
            hardwarecommunication::Port8Bit lbaMidPort;
            hardwarecommunication::Port8Bit lbaHiPort;
            hardwarecommunication::Port8Bit devicePort; /* to define if we want to read from the master or the slave */
            hardwarecommunication::Port8Bit commandPort; /* to say: hey i want to read or i want to write to this port */
            hardwarecommunication::Port8Bit controlPort;
        public:
            
            AdvancedTechnologyAttachment(bool master, common::uint16_t portBase); /* we have multiple ata buses which we pass from the kernel */
            ~AdvancedTechnologyAttachment();
            
            void Identify(); /* we ask to the hard drive information on itself and if it is reachable */
            void Read28(common::uint32_t sectorNum, int count = 512);
            void Write28(common::uint32_t sectorNum, common::uint8_t* data, common::uint32_t count);
            /*
            Indeed, when we write to the hard drive, first the hard drive saves the data in a buffer and only after 
            the buffer has been flushed the data is actually written in the hard drive. So if we don't do it, the data we write
            are actually lost. To avoid to lost data we also use journaling.
            */
            void Flush(); /* to flush the cache of the hard drive */
            
            
        };
        
    }
}

#endif
