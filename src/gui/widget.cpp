 
#include <gui/widget.h>

using namespace myos::common;
using namespace myos::gui;


Widget::Widget(Widget* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                               uint8_t r, uint8_t g, uint8_t b)
: KeyboardEventHandler()
{
    this->parent = parent;
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->r = r;
    this->g = g;
    this->b = b;
    this->Focussable = true;
}

Widget::~Widget()
{
}
            
void Widget::GetFocus(Widget* widget)
{
    /* pass to the parent unless there isn't any */
    if(parent != 0)
        parent->GetFocus(widget);
}

void Widget::ModelToScreen(common::int32_t &x, common::int32_t& y)
{
    /* to get the absolute coordinate we take our own coordinates and we add the one of the parent */
    if(parent != 0)
        parent->ModelToScreen(x,y);
    x += this->x;
    y += this->y;
}
            
void Widget::Draw(GraphicsContext* gc)
{
    int X = 0;
    int Y = 0;
    /* we need the absolute coordinate at this point, we obtain it by passing the X and Y by reference to the ModelToScreen */
    ModelToScreen(X,Y);
    gc->FillRectangle(X,Y,w,h, r,g,b);
}

void Widget::OnMouseDown(common::int32_t x, common::int32_t y, common::uint8_t button)
{
    if(Focussable)
        GetFocus(this);
}

bool Widget::ContainsCoordinate(common::int32_t x, common::int32_t y)
{
    /* 
    x and y must we greater than the coordinates of the widget and smaller than the coordinates summed with height and width 
    this -> x and this -> y are relative coordinates, but it still works because in the CompositeWidget we turn the coordinates
    that we get into relative coordinates as well. So the thing that we pass are also relative coordinates.
    */
    return this->x <= x && x < this->x + this->w
        && this->y <= y && y < this->y + this->h;
}

void Widget::OnMouseUp(common::int32_t x, common::int32_t y, common::uint8_t button)
{
}

void Widget::OnMouseMove(common::int32_t oldx, common::int32_t oldy, common::int32_t newx, common::int32_t newy)
{
}

CompositeWidget::CompositeWidget(Widget* parent,
                   common::int32_t x, common::int32_t y, common::int32_t w, common::int32_t h,
                   common::uint8_t r, common::uint8_t g, common::uint8_t b)
: Widget(parent, x,y,w,h, r,g,b)
{
    focussedChild = 0;
    numChildren = 0;
}

CompositeWidget::~CompositeWidget()
{
}
            
void CompositeWidget::GetFocus(Widget* widget)
{
    this->focussedChild = widget;
    if(parent != 0)
        parent->GetFocus(this);
}

bool CompositeWidget::AddChild(Widget* child)
{
    if(numChildren >= 100)
        return false;
    children[numChildren++] = child;
    return true;
}

/* first draws its own background then draws the background of the children */
void CompositeWidget::Draw(GraphicsContext* gc)
{
    Widget::Draw(gc);
    for(int i = numChildren-1; i >= 0; --i)
        children[i]->Draw(gc);
}


void CompositeWidget::OnMouseDown(int32_t x, int32_t y, common::uint8_t button)
{
    for(int i = 0; i < numChildren; ++i)
        /* if the child contains the coordinate, i pass the event (onMouseDown) to the child */
        if(children[i]->ContainsCoordinate(x - this->x, y - this->y))
        {
            children[i]->OnMouseDown(x - this->x, y - this->y, button);
            /* if i have one child after the other, only the front child is supposed to receive and handle the event so we break */
            break;
        }
}

void CompositeWidget::OnMouseUp(int32_t x, int32_t y, common::uint8_t button)
{
    for(int i = 0; i < numChildren; ++i)
        if(children[i]->ContainsCoordinate(x - this->x, y - this->y))
        {
            children[i]->OnMouseUp(x - this->x, y - this->y, button);
            break;
        }
}

void CompositeWidget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy)
{
    /* firstChild is used to avoid that if we move the mouse within the same widget, it does not execute the event twice on it */
    int firstchild = -1;

    /* 
    we propagate the event twice: one for the event that contains the old coordinate, 
    and one for the event that contains the new coordinate.
    */
    for(int i = 0; i < numChildren; ++i)
        if(children[i]->ContainsCoordinate(oldx - this->x, oldy - this->y))
        {
            children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x, newy - this->y);
            firstchild = i;
            break;
        }

    for(int i = 0; i < numChildren; ++i)
        if(children[i]->ContainsCoordinate(newx - this->x, newy - this->y))
        {
            if(firstchild != i)
                children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x, newy - this->y);
            break;
        }
}

/* a composite widget like the desktop can pass a key strike to the child, so we overwrite these two methods inherited from the keyboard */
void CompositeWidget::OnKeyDown(char str)
{
    if(focussedChild != 0)
        focussedChild->OnKeyDown(str);
}

void CompositeWidget::OnKeyUp(char str)
{
    if(focussedChild != 0)
        focussedChild->OnKeyUp(str);    
}

