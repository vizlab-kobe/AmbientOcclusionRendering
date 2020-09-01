#if 0
#include "Event.h"


namespace local
{

Event::Event( local::Controller& widget ):
    m_widget( widget )
{
    setEventType( kvs::EventBase::KeyPressEvent );
}

void Event::keyPressEvent( kvs::KeyEvent* event )
{
    switch ( event->key() )
    {
    case kvs::Key::i:
    {
        if ( m_widget.isVisible() ) { m_widget.hide(); }
        else { m_widget.show(); }
    }
    default: break;
    }
}

} // end of namespace local
#endif
