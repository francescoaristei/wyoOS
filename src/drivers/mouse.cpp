
#include <drivers/mouse.h>


using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;


void printf(char*);

    MouseEventHandler::MouseEventHandler()
    {
    }
    
    void MouseEventHandler::OnActivate()
    {
    }
    
    void MouseEventHandler::OnMouseDown(uint8_t button)
    {
    }
    
    void MouseEventHandler::OnMouseUp(uint8_t button)
    {
    }
    
    void MouseEventHandler::OnMouseMove(int x, int y)
    {
    }

    MouseDriver::MouseDriver(InterruptManager* manager, MouseEventHandler* handler)
    /* the interrupt handler works on the interrupt with number 0x2C */
    : InterruptHandler(manager, 0x2C),
    dataport(0x60),
    commandport(0x64)
    {
        this->handler = handler;
    }

    MouseDriver::~MouseDriver()
    {
    }
    
    void MouseDriver::Activate()
    {
        offset = 0;
        buttons = 0;

        if(handler != 0)
            handler->OnActivate();
        
        commandport.Write(0xA8); /* activate the mouse interrupt */
        commandport.Write(0x20); // command 0x60 = read controller command byte
        /* set the second bit to true and write it back with write */
        uint8_t status = dataport.Read() | 2;
        commandport.Write(0x60); // command 0x60 = set controller command byte
        dataport.Write(status);

        commandport.Write(0xD4);
        dataport.Write(0xF4);
        dataport.Read();        
    }
    
    uint32_t MouseDriver::HandleInterrupt(uint32_t esp)
    {
        /* read the state from the command port */
        uint8_t status = commandport.Read();

        /* we test weather there is data ro read, only if the sixth bit of the status is 1 only then there is actually data to read */
        if (!(status & 0x20))
            return esp;

        /* read the data into the buffer at the current offset */
        buffer[offset] = dataport.Read();
        
        if(handler == 0)
            return esp;
        
        /* move the offset in the next position of the array */
        offset = (offset + 1) % 3;

        /* if all the three bytes have been written now the offset is zero (circular move) then we can look at the buffer */
        if(offset == 0)
        {
            /* buffer[1] is the movemenet on the x-axes, buffer[2] is the movement on the y-axes but on the opposite direction */
            if(buffer[1] != 0 || buffer[2] != 0)
            {
                handler->OnMouseMove((int8_t)buffer[1], -((int8_t)buffer[2]));
            }

            for(uint8_t i = 0; i < 3; i++)
            {
                if((buffer[0] & (0x1<<i)) != (buttons & (0x1<<i)))
                {
                    if(buttons & (0x1<<i))
                        /* add the movement to the current position to get the new current position */
                        handler->OnMouseUp(i+1);
                    else
                        /* add the movement to the current position to get the new current position */
                        handler->OnMouseDown(i+1);
                }
            }
            buttons = buffer[0];
        }
        
        return esp;
    }
