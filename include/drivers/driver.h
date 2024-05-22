 
#ifndef __MYOS__DRIVERS__DRIVER_H
#define __MYOS__DRIVERS__DRIVER_H

/* Class Driver "extended" by all other driver classes */

namespace myos
{
    namespace drivers
    {

        class Driver
        {
        public:
            Driver();
            ~Driver();
            
            virtual void Activate();
            virtual int Reset();
            virtual void Deactivate();
        };

        /* class driver manager to handle each different driver */
        class DriverManager
        {
        public:
            Driver* drivers[265];
            int numDrivers;
            
        public:
            DriverManager();
            void AddDriver(Driver*);
            
            void ActivateAll();
            
        };
        
    }
}
    
    
#endif