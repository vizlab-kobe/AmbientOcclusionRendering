#pragma once
#include <kvs/Application>
#include <kvs/Screen>
#include "Model.h"
#include "StochasticRenderingCompositor.h"


namespace local
{

class View
{
private:
    local::Model& m_model;
    kvs::Screen m_screen;
    local::StochasticRenderingCompositor m_compositor;

public:
    View( kvs::Application& app, local::Model& model ):
        m_model( model ),
        m_screen( &app ),
        m_compositor( m_screen.scene() )
    {
        this->setup();
        this->show();
    }

    kvs::Screen& screen() { return m_screen; }
    local::StochasticRenderingCompositor& compositor() { return m_compositor; }

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

        // Isosurface
        for ( size_t index = 0; index < 2; ++index )
        {
            const auto* volume = m_model.import( index );
            const auto min_value = volume->minValue();
            const auto max_value = volume->maxValue();
            const auto isovalue = ( max_value + min_value ) * 0.05f;
            auto* object = m_model.isosurface( index, isovalue );
            auto* renderer = m_model.renderer( index );
            m_screen.registerObject( object, renderer );
        }

        m_compositor.setRepetitionLevel( m_model.repeats );
        m_compositor.enableLODControl();
        //m_compositor.setShader( kvs::Shader::BlinnPhong() );
        m_screen.setEvent( &m_compositor );
    }
};

} // end of namespace local
