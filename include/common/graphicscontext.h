#ifndef __MYOS__COMMON__GRAPHICSCONTEXT_H
#define __MYOS__COMMON__GRAPHICSCONTEXT_H

#include <drivers/vga.h>

namespace myos
{
    namespace common
    {
        /* the GraphicsContext class does not exist actually, is just the VGA class */
        typedef myos::drivers::VideoGraphicsArray GraphicsContext;
    }
}

#endif
