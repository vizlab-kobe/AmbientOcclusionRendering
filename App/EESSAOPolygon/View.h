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
        m_screen.setTitle( m_model.title );

        const auto* volume = m_model.import();
        const auto min_value = volume->minValue();
        const auto max_value = volume->maxValue();
        const auto isovalue = ( max_value + min_value ) * 0.02f;
        auto* object = m_model.isosurface( isovalue );
        auto* renderer = m_model.renderer();
        m_screen.registerObject( object, renderer );
    }
};

} // end of namespace local
