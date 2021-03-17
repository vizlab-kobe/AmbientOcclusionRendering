#pragma once
#include <kvs/Application>
#include <kvs/Screen>
#include "Model.h"


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
        m_screen.setBackgroundColor( kvs::RGBColor::White() );
        m_screen.setTitle( "SSAOStochasticUniformGridRenderer" );

        auto* object = m_model.import();
        auto* renderer = m_model.renderer();
        m_screen.registerObject( object, renderer );
    }
};

} // end of namespace local
