#include <hardwarecommunication/pci.h>
#include <drivers/amd_am79c973.h>

using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;


PeripheralComponentInterconnectDeviceDescriptor::PeripheralComponentInterconnectDeviceDescriptor()
{
}

PeripheralComponentInterconnectDeviceDescriptor::~PeripheralComponentInterconnectDeviceDescriptor()
{
}



PeripheralComponentInterconnectController::PeripheralComponentInterconnectController()
/* 0xCFC and 0xCF8 are both standard ports for the PCI */
: dataPort(0xCFC),
  commandPort(0xCF8)
{
}

PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController()
{
}

uint32_t PeripheralComponentInterconnectController::Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset)
{
    /* ID sent to the PCI controller built from the passed parameters */
    uint32_t id =
        /* the first bit needs to be set explicitly to 1 */
        0x1 << 31
        | ((bus & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((function & 0x07) << 8)
        /* 
        we cut of the last two bits of the registeroffset.
        */
        | (registeroffset & 0xFC);
    /* communicate the ID to the PCI controller */
    commandPort.Write(id);
    uint32_t result = dataPort.Read();
    /*
    This result number we get when reading is 4 byte aligned, meaning the bytes are grouped as 32 bits integers.
    We cannot ask to get the location of the third or second bytes of the integer, we can only ask for the full 32 bits integers.
    If we want a bytes that is in the "middle" of the 32 bits integer we can get, we have to ask for the 32 bits integer and extract from there
    the byte we are interested in.
    */
    return result >> (8* (registeroffset % 4));
}

void PeripheralComponentInterconnectController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value)
{
    uint32_t id =
        0x1 << 31
        | ((bus & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((function & 0x07) << 8)
        | (registeroffset & 0xFC);
    commandPort.Write(id);
    dataPort.Write(value); 
}

/* It may be that a device does not have all the 8 possible functions, this is a way to ask a device if it has a certain function */
bool PeripheralComponentInterconnectController::DeviceHasFunctions(common::uint16_t bus, common::uint16_t device)
{
    /* we only need the seventh bit of that (1<<7) because it is the one telling if the device has function or not */
    return Read(bus, device, 0, 0x0E) & (1<<7);
}


void printf(char* str);
void printfHex(uint8_t);

void PeripheralComponentInterconnectController::SelectDrivers(DriverManager* driverManager, myos::hardwarecommunication::InterruptManager* interrupts)
{
    for(int bus = 0; bus < 8; bus++)
    {
        for(int device = 0; device < 32; device++)
        {
            int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
            for(int function = 0; function < numFunctions; function++)
            {
                PeripheralComponentInterconnectDeviceDescriptor dev = GetDeviceDescriptor(bus, device, function);
                
                /* if there is no device on that function than the vendor_id is either all 0s or 1s so we continue */
                if(dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF)
                    continue;
                
                /* iterate over the base address registers */
                for(int barNum = 0; barNum < 6; barNum++)
                {
                    /* 
                    in the GetBaseAddressRegister() the address is set to the higher bits of the BAR, which in case of 
                    the I/O Bars contain the port number 
                    */
                    BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);
                    /* only if the address is set and the type is I/O then we procede */
                    if(bar.address && (bar.type == InputOutput))
                        /* we set the port base value from the device descriptor to the address*/
                        dev.portBase = (uint32_t)bar.address;
                }
                
                /*
                The get driver method is used for the device we want to have the driver for and the driver is connected with the
                interrupt manager
                */
                Driver* driver = GetDriver(dev, interrupts);
                if(driver != 0)
                    /* driver added to the driver manager */
                    driverManager->AddDriver(driver);

                
                printf("PCI BUS ");
                printfHex(bus & 0xFF);
                
                printf(", DEVICE ");
                printfHex(device & 0xFF);

                printf(", FUNCTION ");
                printfHex(function & 0xFF);
                
                printf(" = VENDOR ");
                printfHex((dev.vendor_id & 0xFF00) >> 8);
                printfHex(dev.vendor_id & 0xFF);
                printf(", DEVICE ");
                printfHex((dev.device_id & 0xFF00) >> 8);
                printfHex(dev.device_id & 0xFF);
                printf("\n");
            }
        }
    }
}


BaseAddressRegister PeripheralComponentInterconnectController::GetBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar)
{
    BaseAddressRegister result;
    
    
    /* we are reading the offset 0X0E and we are only interested in the first seven bits of this */
    uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
    /* in the case of the 64 bits base address register there are only 2 bits of the first seven of interest */
    int maxBARs = 6 - (4*headertype);
    /* if we have requested a BAR behind this maximum of 64 we simply return the result */
    if(bar >= maxBARs)
        return result;
    
    /* we read the offset 0x10 plus 4*bar because the BAR has a size of 4 bytes */
    uint32_t bar_value = Read(bus, device, function, 0x10 + 4*bar);
    /* the last bit is for the type so we want to read it */
    result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;
    uint32_t temp;
    
    
    
    if(result.type == MemoryMapping)
    {
        
        switch((bar_value >> 1) & 0x3)
        {
            
            case 0: // 32 Bit Mode
            case 1: // 20 Bit Mode
            case 2: // 64 Bit Mode
                break;
        }
        
    }
    else // InputOutput
    {
        /* 
         we set the address to the bar value but we cancel the last 2 bits (the unused bit and the type) so we obtain the port
        which we will use for communication
        */
        result.address = (uint8_t*)(bar_value & ~0x3);
        result.prefetchable = false;
    }
    
    
    return result;
}



Driver* PeripheralComponentInterconnectController::GetDriver(PeripheralComponentInterconnectDeviceDescriptor dev, InterruptManager* interrupts)
{
    Driver* driver = 0;
    switch(dev.vendor_id)
    {
        /* look for a driver fir for the specific device */
        case 0x1022: // AMD
            switch(dev.device_id)
            {
                case 0x2000: // am79c973
                    printf("AMD am79c973 ");
                    driver = (amd_am79c973*)MemoryManager::activeMemoryManager->malloc(sizeof(amd_am79c973));
                    if(driver != 0)
                        new (driver) amd_am79c973(&dev, interrupts);
                    else
                        printf("instantiation failed");
                    return driver;
                    break;
            }
            break;

        case 0x8086: // Intel
            break;
    }
    
    /* if we don't find a driver fit for the specific device */
    switch(dev.class_id)
    {
        case 0x03: // graphics
            switch(dev.subclass_id)
            {
                case 0x00: // VGA
                    printf("VGA ");
                    break;
            }
            break;
    }
    
    
    return driver;
}



PeripheralComponentInterconnectDeviceDescriptor PeripheralComponentInterconnectController::GetDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t function)
{
    PeripheralComponentInterconnectDeviceDescriptor result;
    
    result.bus = bus;
    result.device = device;
    result.function = function;
    
    result.vendor_id = Read(bus, device, function, 0x00); // offset 00
    result.device_id = Read(bus, device, function, 0x02); // offset 02 etc...

    result.class_id = Read(bus, device, function, 0x0b);
    result.subclass_id = Read(bus, device, function, 0x0a);
    result.interface_id = Read(bus, device, function, 0x09);

    result.revision = Read(bus, device, function, 0x08);
    result.interrupt = Read(bus, device, function, 0x3c);
    
    return result;
}








