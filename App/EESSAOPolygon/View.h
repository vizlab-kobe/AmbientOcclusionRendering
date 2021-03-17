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
        m_screen.setTitle( "MagneticField" );

        auto* volume = m_model.import();
        const auto min_value = volume->minValue();
        const auto max_value = volume->maxValue();
        const auto isovalue = ( max_value + min_value ) * 0.02f;
        m_screen.registerObject( m_model.isosurface( isovalue ), m_model.renderer() );
    }
};

} // end of namespace local
