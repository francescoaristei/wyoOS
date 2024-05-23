 
#include <gui/desktop.h>

using namespace myos;
using namespace myos::common;
using namespace myos::gui;


Desktop::Desktop(common::int32_t w, common::int32_t h,
                common::uint8_t r, common::uint8_t g, common::uint8_t b)
    /* we assume that there is only a top level desktop (first 0), it will start in the upper left corner (second and third 0s) */
:   CompositeWidget(0,0,0, w,h,r,g,b),
    MouseEventHandler()
{
    /* initial mouse position is the center of the screen */
    MouseX = w/2;
    MouseY = h/2;
}

Desktop::~Desktop()
{
}

void Desktop::Draw(common::GraphicsContext* gc)
{
    /* we draw the screen */
    CompositeWidget::Draw(gc);
    
    /* after the screen has been drawn we put the mouse cursor there */
    for(int i = 0; i < 4; i++)
    {
        gc -> PutPixel(MouseX-i, MouseY, 0xFF, 0xFF, 0xFF);
        gc -> PutPixel(MouseX+i, MouseY, 0xFF, 0xFF, 0xFF);
        gc -> PutPixel(MouseX, MouseY-i, 0xFF, 0xFF, 0xFF);
        gc -> PutPixel(MouseX, MouseY+i, 0xFF, 0xFF, 0xFF);
    }
}
            
/* if the mouse does something this is called, and it is translated into the widget form where you get the mouse position and the button */
void Desktop::OnMouseDown(myos::common::uint8_t button)
{
    CompositeWidget::OnMouseDown(MouseX, MouseY, button);
}

void Desktop::OnMouseUp(myos::common::uint8_t button)
{
    CompositeWidget::OnMouseUp(MouseX, MouseY, button);
}

void Desktop::OnMouseMove(int x, int y)
{
    /* relative movements are divided by 4 because it is relatively fast */
    x /= 4;
    y /= 4;
    
    int32_t newMouseX = MouseX + x;
    if(newMouseX < 0) newMouseX = 0;
    if(newMouseX >= w) newMouseX = w - 1;
    
    int32_t newMouseY = MouseY + y;
    if(newMouseY < 0) newMouseY = 0;
    if(newMouseY >= h) newMouseY = h - 1;
    
    /* we call the widget event handler */
    CompositeWidget::OnMouseMove(MouseX, MouseY, newMouseX, newMouseY);
    
    /* we store the new value back in the coordinates */
    MouseX = newMouseX;
    MouseY = newMouseY;
}
