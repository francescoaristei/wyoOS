
#ifndef __MYOS__DRIVERS__MOUSE_H
#define __MYOS__DRIVERS__MOUSE_H

#include <common/types.h>
#include <hardwarecommunication/port.h>
#include <drivers/driver.h>
#include <hardwarecommunication/interrupts.h>

namespace myos
{
    namespace drivers
    {
    
        class MouseEventHandler
        {
        public:
            MouseEventHandler();

            virtual void OnActivate();
            virtual void OnMouseDown(myos::common::uint8_t button);
            virtual void OnMouseUp(myos::common::uint8_t button);
            virtual void OnMouseMove(int x, int y);
        };
        
        
        class MouseDriver : public myos::hardwarecommunication::InterruptHandler, public Driver
        {
            myos::hardwarecommunication::Port8Bit dataport;
            myos::hardwarecommunication::Port8Bit commandport;
            /* 3 bytes buffer */
            myos::common::uint8_t buffer[3];
            myos::common::uint8_t offset;
            /* the transition from the mouse always give the current state, but to understand if the cursor has move
            we need to compare the current state with the previous, hence we save here the previous state of the mouse */
            myos::common::uint8_t buttons;

            MouseEventHandler* handler;
        public:
            MouseDriver(myos::hardwarecommunication::InterruptManager* manager, MouseEventHandler* handler);
            ~MouseDriver();
            virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);
            virtual void Activate();
        };

    }
}
    
#endif
