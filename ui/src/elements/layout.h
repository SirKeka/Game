#pragma once
#include "panel.h"

class Layout : public Panel
{
public:
    UiElement** elements;
    enum Type : u8 { Horisontal, Vertical, Grid } type;
    enum Direction : u8 { LeftToRight, RightToLeft, TopToBottom, BottomToTop } direction;

    constexpr Layout(UiElement* parent = nullptr) : Panel(), elements() {}
    constexpr Layout(const char* name, FVec2 size, const FVec4& colour, Type type, UiElement* parent = nullptr) : Panel(name, size, colour, parent), elements(), type(type), direction()  {}
    ~Layout();

    void AddElement(UiElement* element, u32 stretch = 0); 
};