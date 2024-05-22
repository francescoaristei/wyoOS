
#include <drivers/driver.h>
using namespace myos::drivers;
 
Driver::Driver()
{
}

Driver::~Driver()
{
}
        
void Driver::Activate()
{
}

/*
When the OS is started you enumerate the hardware, ut you don't know the state of the hardware, so you reset them and wait
a bit so that you know the state of the hardware, is a matter of security. What if the booloader didn't clean up perfectly the last run of the OS? 
To avoid inconsistent state we reset everything. Reset() will tell how long we should wait until we restart the boot sequence. 
*/
int Driver::Reset()
{
    return 0;
}

void Driver::Deactivate()
{
}




DriverManager::DriverManager()
{
    numDrivers = 0;
}

void DriverManager::AddDriver(Driver* drv)
{
    drivers[numDrivers] = drv;
    numDrivers++;
}

void DriverManager::ActivateAll()
{
    for(int i = 0; i < numDrivers; i++)
        drivers[i]->Activate();
}
        
