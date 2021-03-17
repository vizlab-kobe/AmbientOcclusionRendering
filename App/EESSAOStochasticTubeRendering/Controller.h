#pragma once
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/OrientationAxis>
#include <kvs/ColorMapBar>
#include <kvs/TransferFunctionEditor>
#include <kvs/KeyPressEventListener>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/PaintEventListener>
#include "Model.h"
#include "View.h"


namespace local
{

class Controller
{
private:
    local::Model& m_model;
    local::View& m_view;

    // Widgets
    kvs::CheckBox m_ao_check_box;
    kvs::CheckBox m_lod_check_box;
    kvs::Slider m_repeat_slider;
    kvs::Slider m_radius_slider;
    kvs::Slider m_points_slider;
    kvs::Slider m_edge_slider;
    kvs::OrientationAxis m_axis;
    kvs::ColorMapBar m_cmap_bar;
    kvs::TransferFunctionEditor m_editor;

    // Events
    kvs::ScreenCaptureEvent m_capture_event;
    kvs::TargetChangeEvent m_target_change_event;
    kvs::KeyPressEventListener m_key_event;
    kvs::PaintEventListener m_paint_event;

public:
    Controller( local::Model& model, local::View& view ):
        m_model( model ),
        m_view( view ),
        m_ao_check_box( &view.screen() ),
        m_lod_check_box( &view.screen() ),
        m_repeat_slider( &view.screen() ),
        m_radius_slider( &view.screen() ),
        m_points_slider( &view.screen() ),
        m_edge_slider( &view.screen() ),
        m_axis( &view.screen(), view.screen().scene() ),
        m_cmap_bar( &view.screen() ),
        m_editor( &view.screen() )
    {
        // AO
        m_ao_check_box.setCaption( "AO" );
        m_ao_check_box.setState( m_model.ao );
        m_ao_check_box.setMargin( 10 );
        m_ao_check_box.stateChanged( [&] ()
        {
            m_model.ao = m_ao_check_box.state();
            m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
        } );

        // LOD
        m_lod_check_box.setCaption( "LOD" );
        m_lod_check_box.setState( m_model.lod );
        m_lod_check_box.setMargin( 10 );
        m_lod_check_box.anchorToBottom( &m_ao_check_box );
        m_lod_check_box.stateChanged( [&] ()
        {
            m_model.lod = m_lod_check_box.state();
            auto* scene = m_view.screen().scene();
            if ( m_model.ao )
            {
                auto* renderer = local::Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
                renderer->setLODControlEnabled( m_model.lod );
            }
            else
            {
                auto* renderer = local::Model::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
                renderer->setLODControlEnabled( m_model.lod );
            }
        } );

        // Repeat
        m_repeat_slider.setCaption( "Repeats: " + kvs::String::ToString( m_model.repeats ) );
        m_repeat_slider.setValue( m_model.repeats );
        m_repeat_slider.setRange( 1, 100 );
        m_repeat_slider.setMargin( 10 );
        m_repeat_slider.anchorToBottom( &m_lod_check_box );
        m_repeat_slider.sliderMoved( [&] ()
        {
            m_model.repeats = m_repeat_slider.value();
            m_repeat_slider.setCaption( "Repeats: " + kvs::String::From( m_model.repeats ) );
        } );
        m_repeat_slider.sliderReleased( [&] ()
        {
            auto* scene = m_view.screen().scene();
            if ( m_model.ao )
            {
                auto* renderer = local::Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
                renderer->setRepetitionLevel( m_model.repeats );
            }
            else
            {
                auto* renderer = local::Model::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
                renderer->setRepetitionLevel( m_model.repeats );
            }
        } );

        // Radius
        m_radius_slider.setCaption( "Radius: " + kvs::String::ToString( m_model.radius ) );
        m_radius_slider.setValue( m_model.radius );
        m_radius_slider.setRange( 0.1, 5.0 );
        m_radius_slider.setMargin( 10 );
        m_radius_slider.anchorToBottom( &m_repeat_slider );
        m_radius_slider.sliderMoved( [&] ()
        {
            const float min_value = m_radius_slider.minValue();
            const float max_value = m_radius_slider.maxValue();
            const float v = int( m_radius_slider.value() * 10 ) * 0.1f;
            m_model.radius = kvs::Math::Clamp( v, min_value, max_value );
            m_radius_slider.setCaption( "Radius: " + kvs::String::From( m_model.radius ) );
        } );
        m_radius_slider.sliderReleased( [&] ()
        {
            if ( m_model.ao )
            {
                m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
            }
        } );

        // Points
        m_points_slider.setCaption( "Points: " + kvs::String::From( m_model.points ) );
        m_points_slider.setValue( m_model.points );
        m_points_slider.setRange( 1, 256 );
        m_points_slider.setMargin( 10 );
        m_points_slider.anchorToBottom( &m_radius_slider );
        m_points_slider.sliderMoved( [&] ()
        {
            m_model.points = int( m_points_slider.value() );
            m_points_slider.setCaption( "Points: " + kvs::String::From( m_model.points ) );
        } );
        m_points_slider.sliderReleased( [&] ()
        {
            if ( m_model.ao )
            {
                m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
            }
        } );

        // Edge
        m_edge_slider.setCaption( "Edge: " + kvs::String::From( m_model.edge ) );
        m_edge_slider.setValue( m_model.edge );
        m_edge_slider.setRange( 0, 5 );
        m_edge_slider.setMargin( 10 );
        m_edge_slider.anchorToBottom( &m_points_slider );
        m_edge_slider.sliderMoved( [&] ()
        {
            const float v = int( m_edge_slider.value() * 10 ) * 0.1f;
            m_model.edge = v;
            m_edge_slider.setCaption( "Edge: " + kvs::String::From( m_model.edge ) );
        } );
        m_edge_slider.sliderReleased( [&] ()
        {
            if ( m_model.ao )
            {
                m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
            }
        });

        // Axis
        m_axis.setBoxType( kvs::OrientationAxis::SolidBox );
        m_axis.anchorToBottomLeft();
        m_axis.show();

        // Colormap bar
        m_cmap_bar.setCaption( "Velocity Magnitude" );
        m_cmap_bar.setColorMap( m_model.tfunc.colorMap() );
        m_cmap_bar.anchorToBottomRight();

        // Transfer function editor
        m_editor.setTransferFunction( m_model.tfunc );
        m_editor.show();
        m_editor.apply( [&] ( kvs::TransferFunction tfunc )
        {
            m_model.tfunc = tfunc;
            auto* scene = m_view.screen().scene();
            if ( m_model.ao )
            {
                auto* renderer = local::Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
                renderer->setTransferFunction( tfunc );
            }
            else
            {
                auto* renderer = local::Model::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
                renderer->setTransferFunction( tfunc );
            }
            m_cmap_bar.setColorMap( tfunc.colorMap() );
            m_view.screen().redraw();
        } );

        // Key press event
        m_key_event.update( [&] ( kvs::KeyEvent* event )
        {
            switch ( event->key() )
            {
            case kvs::Key::i:
            {
                if ( this->isVisible() ) { this->hide(); }
                else { this->show(); }
                m_view.screen().redraw();
                break;
            }
            default: break;
            }
        } );

        // Paint event
        m_paint_event.update( [&] ()
        {
            static size_t counter = 1;
            static float timer = 0.0f;

            timer += m_view.screen().scene()->renderer("Renderer")->timer().msec();
            if ( counter++ == 50 )
            {
                std::cout << "Rendering time: " << timer / counter << " [msec]" << std::endl;
                counter = 1;
                timer = 0.0f;
            }
        } );

        m_view.screen().addEvent( &m_paint_event );
        m_view.screen().addEvent( &m_key_event );
        m_view.screen().addEvent( &m_capture_event );
        m_view.screen().addEvent( &m_target_change_event );

        this->show();
    }


protected:
    bool isVisible() const
    {
        return m_ao_check_box.isVisible();
    }

    void show()
    {
        m_ao_check_box.show();
        m_lod_check_box.show();
        m_repeat_slider.show();
        m_radius_slider.show();
        m_points_slider.show();
        m_edge_slider.show();
        m_axis.show();
        m_cmap_bar.show();
    }

    void hide()
    {
        m_ao_check_box.hide();
        m_lod_check_box.hide();
        m_repeat_slider.hide();
        m_radius_slider.hide();
        m_points_slider.hide();
        m_edge_slider.hide();
        m_axis.hide();
        m_cmap_bar.hide();
    }
};

} // end of namespace local
