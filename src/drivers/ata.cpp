#include <drivers/ata.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;


void printf(char* str);
void printfHex(uint8_t);

AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(bool master, common::uint16_t portBase)
:   dataPort(portBase),
    errorPort(portBase + 0x1),
    sectorCountPort(portBase + 0x2),
    lbaLowPort(portBase + 0x3),
    lbaMidPort(portBase + 0x4),
    lbaHiPort(portBase + 0x5),
    devicePort(portBase + 0x6),
    commandPort(portBase + 0x7),
    controlPort(portBase + 0x206)
{
    this->master = master;
}

AdvancedTechnologyAttachment::~AdvancedTechnologyAttachment()
{
}
            
void AdvancedTechnologyAttachment::Identify()
{
    /* we write to the device port: hey i want to talk to the master/slave */
    devicePort.Write(master ? 0xA0 : 0xB0);
    controlPort.Write(0);
    
    /* clears a flag, not important */
    devicePort.Write(0xA0);

    /* read the state of the master (to read the state always speak with the master) */
    uint8_t status = commandPort.Read();
    if(status == 0xFF) /* if 255 return because it means there are no device to that bus */
        return;
    
    
    devicePort.Write(master ? 0xA0 : 0xB0);
    sectorCountPort.Write(0);
    lbaLowPort.Write(0);
    lbaMidPort.Write(0);
    lbaHiPort.Write(0);
    commandPort.Write(0xEC); // identify command
    
    
    status = commandPort.Read();
    if(status == 0x00) /* again, no device, return */
        return;
    
    /* take sometime before the device sends us the requested info (is ready), update the status until the device is ready */
    while(((status & 0x80) == 0x80)
       && ((status & 0x01) != 0x01)) /* if set to 0x01 we have an error */
        status = commandPort.Read();
        
    if(status & 0x01)
    {
        printf("ERROR");
        return;
    }
    
    /* data is ready, we can read it */
    for(int i = 0; i < 256; i++)
    {
        uint16_t data = dataPort.Read();
        char *text = "  \0";
        text[0] = (data >> 8) & 0xFF;
        text[1] = data & 0xFF;
        printf(text);
    }
    printf("\n");
}

void AdvancedTechnologyAttachment::Read28(common::uint32_t sectorNum, int count)
{
    if(sectorNum > 0x0FFFFFFF)
        return;
    
    /* which drive you want to talk to */
    devicePort.Write( (master ? 0xE0 : 0xF0) | ((sectorNum & 0x0F000000) >> 24) );
    errorPort.Write(0);
    /* how many sectors we want to read */
    sectorCountPort.Write(1);
    /* the number of the sector we want to read from  */
    lbaLowPort.Write(  sectorNum & 0x000000FF );
    lbaMidPort.Write( (sectorNum & 0x0000FF00) >> 8);
    lbaLowPort.Write( (sectorNum & 0x00FF0000) >> 16 );
    commandPort.Write(0x20);
    
    /*
    When we want to read it may take sometime until the hard drive is ready to give us the data, therefore we loop until 
    the device is not ready.
    */
    uint8_t status = commandPort.Read();
    while(((status & 0x80) == 0x80)
       && ((status & 0x01) != 0x01))
        status = commandPort.Read();
        
    if(status & 0x01)
    {
        printf("ERROR");
        return;
    }
    
    
    printf("Reading ATA Drive: ");
    
    /* we want to write the read bytes in the data array wdata */
    for(int i = 0; i < count; i += 2)
    {
        uint16_t wdata = dataPort.Read();
        
        char *text = "  \0";
        text[0] = wdata & 0xFF;
        
        
        if(i+1 < count)
            text[1] = (wdata >> 8) & 0xFF;
        else
            text[1] = '\0';
        
        printf(text);
    }    
    
    /* if we read from an hard drive, the hard drive request us to read one full sector, so we continue reading */
    for(int i = count + (count%2); i < 512; i += 2)
        dataPort.Read();
}

void AdvancedTechnologyAttachment::Write28(common::uint32_t sectorNum, common::uint8_t* data, common::uint32_t count)
{
    /* first four bits must be zero, we cannot address something larger than 28 bits. */
    if(sectorNum > 0x0FFFFFFF)
        return;
    if(count > 512)
        return;
    
    
    /* when we select master/slave we have to select E0 or F0.  */
    devicePort.Write( (master ? 0xE0 : 0xF0) | ((sectorNum & 0x0F000000) >> 24) ); /* 
                                                                                    Reading/Writing in 28 bits mode: 
                                                                                    we have 3 ports of 8 bits each, 
                                                                                    therefore 24 bits to send, 
                                                                                    and 4 bits are left for the address, 
                                                                                    so we put this into the device port also
                                                                                    */
    errorPort.Write(0);
    sectorCountPort.Write(1); /* write a single sector */
    /* split the sector number and puts it into the three ports */
    lbaLowPort.Write(  sectorNum & 0x000000FF ); /* low 8 bits into this port */
    lbaMidPort.Write( (sectorNum & 0x0000FF00) >> 8);
    lbaLowPort.Write( (sectorNum & 0x00FF0000) >> 16 );
    commandPort.Write(0x30); /* 0x30 is the write command */
    
    
    printf("Writing to ATA Drive: ");

    for(int i = 0; i < count; i += 2)
    {
        /* we take the i-th byte from the data array, if the next is also there we put it in there also */
        uint16_t wdata = data[i];
        if(i+1 < count)
            wdata |= ((uint16_t)data[i+1]) << 8;
        dataPort.Write(wdata);

        
        
        char *text = "  \0";
        text[0] = (wdata >> 8) & 0xFF;
        text[1] = wdata & 0xFF;
        printf(text);
    }
    
    /*
    The device expects always to send as many bytes as there are in a sector, otherwise it gives an interrupt, 
    so if we write less bytes then the number of bytes of a sector, we fill the rest of the sector with 0s.
    If count is an odd number, the first one has already been written with the 0, so we add count % 2.
    */
    for(int i = count + (count%2); i < 512; i += 2)
        dataPort.Write(0x0000);

}

void AdvancedTechnologyAttachment::Flush()
{
    devicePort.Write( master ? 0xE0 : 0xF0 );
    commandPort.Write(0xE7); /* 0xE7 is the flush command */

    uint8_t status = commandPort.Read();
    if(status == 0x00)
        return;
    
    /* wait while the device is flushing */
    while(((status & 0x80) == 0x80)
       && ((status & 0x01) != 0x01))
        status = commandPort.Read();
        
    if(status & 0x01)
    {
        printf("ERROR");
        return;
    }
}
            