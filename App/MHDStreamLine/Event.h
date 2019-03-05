#pragma once
#include "Widget.h"
#include <kvs/EventListener>


namespace local
{

class Event : public kvs::EventListener
{
private:
    local::Widget& m_widget;

public:
    Event( local::Widget& widget );
    void keyPressEvent( kvs::KeyEvent* event );
};

} // end of namespace local
