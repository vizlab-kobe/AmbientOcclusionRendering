#if 0
#pragma once
#include "Controller.h"
#include <kvs/EventListener>


namespace local
{

class Event : public kvs::EventListener
{
private:
    local::Controller& m_widget;

public:
    Event( local::Controller& widget );
    void keyPressEvent( kvs::KeyEvent* event );
};

} // end of namespace local
#endif
