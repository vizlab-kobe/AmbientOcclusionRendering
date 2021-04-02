#pragma once
#include <kvs/Application>
#include <kvs/Screen>
#include "Model.h"
#include <AmbientOcclusionRendering/Lib/SSAOStochasticRenderingCompositor.h>


namespace local
{

class View
{
public:
    using Compositor = AmbientOcclusionRendering::SSAOStochasticRenderingCompositor;

private:
    local::Model& m_model;
    kvs::Screen m_screen;
    Compositor m_compositor;

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
    Compositor& compositor() { return m_compositor; }

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

        // Raycasting
        {
            const auto method = Input::Method::Raycasting;
            auto* object = m_model.import( method );
            auto* renderer = m_model.renderer( method );
            m_screen.registerObject( object, renderer );
        }

        // Streamline
        {
            const auto method = Input::Method::Streamline;
            auto* object = m_model.streamline();
            auto* renderer = m_model.renderer( method );
            m_screen.registerObject( object, renderer );
        }

        m_compositor.setRepetitionLevel( m_model.repeats );
        m_compositor.enableLODControl();
        //m_compositor.setShader( kvs::Shader::BlinnPhong() );
        m_screen.setEvent( &m_compositor );
    }
};

} // end of namespace local
