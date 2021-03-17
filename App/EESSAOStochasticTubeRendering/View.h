#pragma once
#include "Model.h"
#include <kvs/Application>
#include <kvs/Screen>


namespace local
{

class View
{
private:
    local::Model& m_model;
    kvs::Screen m_screen;

public:
    View( kvs::Application& app, local::Model& model ):
        m_model( model ),
        m_screen( &app )
    {
        this->setup();
        this->show();
    }

    kvs::Screen& screen() { return m_screen; }

    void show()
    {
        m_screen.show();
    }

    void redraw()
    {
        m_screen.redraw();
    }

    void setup()
    {
        m_screen.setTitle( "MHD Streamline" );
        m_screen.setBackgroundColor( kvs::RGBColor::White() );

        auto* object = m_model.streamline();
        auto* renderer = m_model.renderer();
        m_screen.registerObject( object, renderer );
    }
};

} // end of namespace local
