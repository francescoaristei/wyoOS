 
#include <gui/window.h>

using namespace myos::common;
using namespace myos::gui;

Window::Window(Widget* parent,
            common::int32_t x, common::int32_t y, common::int32_t w, common::int32_t h,
            common::uint8_t r, common::uint8_t g, common::uint8_t b)
: CompositeWidget(parent, x,y,w,h, r,g,b)
{
    Dragging = false;
}

Window::~Window()
{
}

void Window::OnMouseDown(common::int32_t x, common::int32_t y, common::uint8_t button)
{
    /* only if we are clicking the mouse we want to drag the window */
    Dragging = button == 1;
    CompositeWidget::OnMouseDown(x,y,button);
}

void Window::OnMouseUp(common::int32_t x, common::int32_t y, common::uint8_t button)
{
    Dragging = false;
    CompositeWidget::OnMouseUp(x,y,button);
}

void Window::OnMouseMove(common::int32_t oldx, common::int32_t oldy, common::int32_t newx, common::int32_t newy)
{
    /* if we want to drag the window then we change the x and y coordinates */
    if(Dragging)
    {
        this->x += newx-oldx;
        this->y += newy-oldy;
    }

    /* the widget that the windows contains will never be notified that we want to move the window if we don't call the parent method */
    CompositeWidget::OnMouseMove(oldx,oldy,newx, newy);

}

    
