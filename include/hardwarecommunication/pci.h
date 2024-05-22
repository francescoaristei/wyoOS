#ifndef __MYOS__HARDWARECOMMUNICATION__PCI_H
#define __MYOS__HARDWARECOMMUNICATION__PCI_H

#include <hardwarecommunication/port.h>
#include <drivers/driver.h>
#include <common/types.h>
#include <hardwarecommunication/interrupts.h>

#include <memorymanagement.h>

namespace myos
{
    namespace hardwarecommunication
    {

        enum BaseAddressRegisterType
        {   /* types of base address registers, used for the last bit of the BAR */
            MemoryMapping = 0,
            InputOutput = 1
        };
        
        
        class BaseAddressRegister
        {
        public:
            bool prefetchable;
            myos::common::uint8_t* address;
            myos::common::uint32_t size;
            BaseAddressRegisterType type;
        };
        
        
        /* class to store the information about the device (given by the PCI) */
        class PeripheralComponentInterconnectDeviceDescriptor
        {
        public:
            myos::common::uint32_t portBase;
            myos::common::uint32_t interrupt;
            
            myos::common::uint16_t bus;
            myos::common::uint16_t device;
            myos::common::uint16_t function;

            myos::common::uint16_t vendor_id;
            myos::common::uint16_t device_id;
            
            myos::common::uint8_t class_id;
            myos::common::uint8_t subclass_id;
            myos::common::uint8_t interface_id;

            myos::common::uint8_t revision;
            
            PeripheralComponentInterconnectDeviceDescriptor();
            ~PeripheralComponentInterconnectDeviceDescriptor();
            
        };


        class PeripheralComponentInterconnectController
        {
            /* ports to control/communicate with the PCI */
            Port32Bit dataPort;
            Port32Bit commandPort;
            
        public:
            PeripheralComponentInterconnectController();
            ~PeripheralComponentInterconnectController();
            
            /*
            we need to pass the bus number, device number and function number. We will be able to read data from the functions of the 
            device. Each function has some memory with standardized memory space. So we just have to read certain offset of that memory, where
            we will find the classID, subclassID etc. Se we also have to pass the offset of the data we want to read (there is an offset for the
            classID, one for the subclass ID etc...).
            */
            myos::common::uint32_t Read(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function, myos::common::uint32_t registeroffset);
            void Write(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function, myos::common::uint32_t registeroffset, myos::common::uint32_t value);
            bool DeviceHasFunctions(myos::common::uint16_t bus, myos::common::uint16_t device);
            
            /* 
            DriverManager --> PCI Controller (ask info (classID etc...) to PCI controller)
            PCI Controller --> DriverManager (send to the DriverManager info) (with the driver code ?)
            DriverManager add driver to list of drivers in order to use it in the kernel.
            */
            void SelectDrivers(myos::drivers::DriverManager* driverManager, myos::hardwarecommunication::InterruptManager* interrupts);
            myos::drivers::Driver* GetDriver(PeripheralComponentInterconnectDeviceDescriptor dev, myos::hardwarecommunication::InterruptManager* interrupts);
            PeripheralComponentInterconnectDeviceDescriptor GetDeviceDescriptor(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function);
            BaseAddressRegister GetBaseAddressRegister(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function, myos::common::uint16_t bar);
        };

    }
}
    
#endif