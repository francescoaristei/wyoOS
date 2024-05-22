
#include <drivers/vga.h>

using namespace myos::common;
using namespace myos::drivers;

           
            
VideoGraphicsArray::VideoGraphicsArray() : 
    miscPort(0x3c2),
    crtcIndexPort(0x3d4),
    crtcDataPort(0x3d5),
    sequencerIndexPort(0x3c4),
    sequencerDataPort(0x3c5),
    graphicsControllerIndexPort(0x3ce),
    graphicsControllerDataPort(0x3cf),
    attributeControllerIndexPort(0x3c0),
    attributeControllerReadPort(0x3c1),
    attributeControllerWritePort(0x3c0),
    attributeControllerResetPort(0x3da)
{
}

VideoGraphicsArray::~VideoGraphicsArray()
{
}
            
void VideoGraphicsArray::WriteRegisters(uint8_t* registers)
{
    //  misc
    miscPort.Write(*(registers++));
    
    // sequencer
    for(uint8_t i = 0; i < 5; i++)
    {
        sequencerIndexPort.Write(i);
        sequencerDataPort.Write(*(registers++));
    }
    
    // cathode ray tube controller
    /* cathode ray tube needs to be unlocked, send data, and lock it, it is done for security reasons. Here we unlock it. */
    crtcIndexPort.Write(0x03);
    /* we get the old value and we set the first bit to 1 writing it back to that index */
    crtcDataPort.Write(crtcDataPort.Read() | 0x80);
    crtcIndexPort.Write(0x11);
    /* for the seventheenth index we set the first bit to 0 */
    crtcDataPort.Write(crtcDataPort.Read() & ~0x80);
    
    /* 
    In the register written in 0x03 we also set the first bit to 1 and the register written in 0x11 we set the first bit to 0
    just to make sure that we don't overwrite it again
     */
    registers[0x03] = registers[0x03] | 0x80;
    registers[0x11] = registers[0x11] & ~0x80;
    
    for(uint8_t i = 0; i < 25; i++)
    {
        crtcIndexPort.Write(i);
        crtcDataPort.Write(*(registers++));
    }
    
    // graphics controller
    for(uint8_t i = 0; i < 9; i++)
    {
        graphicsControllerIndexPort.Write(i);
        graphicsControllerDataPort.Write(*(registers++));
    }
    
    // attribute controller
    for(uint8_t i = 0; i < 21; i++)
    {
        attributeControllerResetPort.Read();
        attributeControllerIndexPort.Write(i);
        attributeControllerWritePort.Write(*(registers++));
    }
    
    /* we reset the attribute controller again and write 0x20 to the attribute controller index port */
    attributeControllerResetPort.Read();
    attributeControllerIndexPort.Write(0x20);
    
}

bool VideoGraphicsArray::SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth)
{
    /* 8 bits color depth */
    return width == 320 && height == 200 && colordepth == 8;
}

bool VideoGraphicsArray::SetMode(uint32_t width, uint32_t height, uint32_t colordepth)
{
    if(!SupportsMode(width, height, colordepth))
        return false;
    
    /* these are the sequence of codes that we have to write in the registers (memory) allocated to the VGA */
    unsigned char g_320x200x256[] =
    {
        /* MISC */
            0x63,
        /* SEQ */
            0x03, 0x01, 0x0F, 0x00, 0x0E,
        /* CRTC */
            0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
            0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
            0xFF,
        /* GC */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
            0xFF,
        /* AC */
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x41, 0x00, 0x0F, 0x00, 0x00
    };
    
    WriteRegisters(g_320x200x256);
    return true;
}


uint8_t* VideoGraphicsArray::GetFrameBufferSegment()
{
    /* we are looking for index number 6 in the graphic controller */
    graphicsControllerIndexPort.Write(0x06);
    /*
    We shift by two because we are only interested in bits number 3 and 4 and then we take the bitwise AND with bit
    number 3 so that all the other bits are killed
    */
    uint8_t segmentNumber = graphicsControllerDataPort.Read() & (3<<2);
    switch(segmentNumber)
    {
        default:
        /* if it is 0 then we need to write our data to the memory location 0x00000 */
        case 0<<2: return (uint8_t*)0x00000;
        case 1<<2: return (uint8_t*)0xA0000;
        case 2<<2: return (uint8_t*)0xB0000;
        case 3<<2: return (uint8_t*)0xB8000;
    }
}
            
void VideoGraphicsArray::PutPixel(int32_t x, int32_t y,  uint8_t colorIndex)
{
    if(x < 0 || 320 <= x
    || y < 0 || 200 <= y)
        return;
        
    /* we get the current buffer segment which is where to put the pixel */
    uint8_t* pixelAddress = GetFrameBufferSegment() + 320*y + x;
    *pixelAddress = colorIndex;
}

uint8_t VideoGraphicsArray::GetColorIndex(uint8_t r, uint8_t g, uint8_t b)
{
    if(r == 0x00 && g == 0x00 && b == 0x00) return 0x00; // black
    if(r == 0x00 && g == 0x00 && b == 0xA8) return 0x01; // blue
    if(r == 0x00 && g == 0xA8 && b == 0x00) return 0x02; // green
    if(r == 0xA8 && g == 0x00 && b == 0x00) return 0x04; // red
    if(r == 0xFF && g == 0xFF && b == 0xFF) return 0x3F; // white
    return 0x00;
}
           
void VideoGraphicsArray::PutPixel(int32_t x, int32_t y,  uint8_t r, uint8_t g, uint8_t b)
{
    /* we take the color index for a certain rgb color and we use it in the put pixel method */
    PutPixel(x,y, GetColorIndex(r,g,b));
}

void VideoGraphicsArray::FillRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h,   uint8_t r, uint8_t g, uint8_t b)
{
    for(int32_t Y = y; Y < y+h; Y++)
        for(int32_t X = x; X < x+w; X++)
            PutPixel(X, Y, r, g, b);
}

