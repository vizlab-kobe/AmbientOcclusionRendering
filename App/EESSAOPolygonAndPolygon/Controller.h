#pragma once
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/RadioButton>
#include <kvs/RadioButtonGroup>
#include <kvs/ColorMapBar>
#include <kvs/OrientationAxis>
#include <kvs/TransferFunctionEditor>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
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
    kvs::CheckBox m_ao_check_box{ &m_view.screen() };
    kvs::CheckBox m_lod_check_box{ &m_view.screen() };
    kvs::Slider m_repeat_slider{ &m_view.screen() };
    kvs::Slider m_radius_slider{ &m_view.screen() };
    kvs::Slider m_points_slider{ &m_view.screen() };
    kvs::Slider m_edge_slider{ &m_view.screen() };
    kvs::Slider m_opacity_sliders[2] = { { &m_view.screen() }, { &m_view.screen() } };
    kvs::Slider m_isovalue_sliders[2] = { { &m_view.screen() }, { &m_view.screen() } };
    kvs::OrientationAxis m_axis{ &m_view.screen(), m_view.screen().scene() };
    kvs::ColorMapBar m_cmap_bars[2] = { kvs::ColorMapBar( &m_view.screen() ), kvs::ColorMapBar( &m_view.screen() ) };

    // Events
    kvs::ScreenCaptureEvent m_capture_event;
    kvs::TargetChangeEvent m_target_change_event;
    kvs::KeyPressEventListener m_key_event;
    kvs::PaintEventListener m_paint_event;

public:
    Controller( local::Model& model, local::View& view ):
        m_model( model ),
        m_view( view )
    {
        // Initial values for isosurface.
//        auto* isosurface_volume = m_model.import( Input::MappingMethod::Isosurface );
//        const auto isosurface_min_value = isosurface_volume->minValue();
//        const auto isosurface_max_value = isosurface_volume->maxValue();
//        const auto isovalue = ( isosurface_max_value + isosurface_min_value ) * 0.02f;
        kvs::Real32 min_values[2] = { 0.0f, 0.0f };
        kvs::Real32 max_values[2] = { 0.0f, 0.0f };
        kvs::Real32 isovalues[2] = { 0.0f, 0.0f };
        for ( size_t index = 0; index < 2; ++index )
        {
            auto* volume = m_model.import( index );
            min_values[index] = volume->minValue();
            max_values[index] = volume->maxValue();
            isovalues[index] = ( volume->maxValue() + volume->minValue() ) * 0.5f;
        }

        // AO
        m_ao_check_box.setCaption( "AO" );
        m_ao_check_box.setState( m_model.ao );
        m_ao_check_box.stateChanged( [&] ()
        {
            m_model.ao = m_ao_check_box.state();
        } );

        // LOD
        m_lod_check_box.setCaption( "LOD" );
        m_lod_check_box.setState( m_model.lod );
        m_lod_check_box.stateChanged( [&] ()
        {
            m_model.lod = m_lod_check_box.state();
            m_view.compositor().setEnabledLODControl( m_model.lod );
        } );

        // Repeat
        m_repeat_slider.setCaption( "Repeats: " + kvs::String::ToString( m_model.repeats ) );
        m_repeat_slider.setValue( m_model.repeats );
        m_repeat_slider.setRange( 1, 100 );
        m_repeat_slider.sliderMoved( [&] ()
        {
            m_model.repeats = m_repeat_slider.value();
            m_repeat_slider.setCaption( "Repeats: " + kvs::String::From( m_model.repeats ) );
        } );
        m_repeat_slider.sliderReleased( [&] ()
        {
            m_view.compositor().setRepetitionLevel( m_model.repeats );
        } );

        // Radius
        m_radius_slider.setCaption( "Radius: " + kvs::String::ToString( m_model.radius ) );
        m_radius_slider.setValue( m_model.radius );
        m_radius_slider.setRange( 0.1, 5.0 );
        m_radius_slider.sliderMoved( [&] ()
        {
            const float min_rad = m_radius_slider.minValue();
            const float max_rad = m_radius_slider.maxValue();
            const float v = int( m_radius_slider.value() * 2 ) * 0.5f;
            m_model.radius = kvs::Math::Clamp( v, min_rad, max_rad );
            m_radius_slider.setCaption( "Radius: " + kvs::String::From( m_model.radius ) );
        } );
        m_radius_slider.sliderReleased( [&] ()
        {
            m_view.compositor().setSamplingSphereRadius( m_model.radius );
        } );

        // Points
        m_points_slider.setCaption( "Points: " + kvs::String::From( m_model.points ) );
        m_points_slider.setValue( m_model.points );
        m_points_slider.setRange( 1, 256 );
        m_points_slider.sliderMoved( [&] ()
        {
            m_model.points = int( m_points_slider.value() );
            m_points_slider.setCaption( "Points: " + kvs::String::From( m_model.points ) );
        } );
        m_points_slider.sliderReleased( [&] ()
        {
            m_view.compositor().setNumberOfSamplingPoints( m_model.points );
        } );

        // Edge
        m_edge_slider.setCaption( "Edge: " + kvs::String::ToString( m_model.edge ) );
        m_edge_slider.setValue( m_model.edge );
        m_edge_slider.setRange( 0.0, 5.0 );
        m_edge_slider.sliderMoved( [&] ()
        {
            float v = int( m_edge_slider.value() * 10 ) * 0.1f;
            m_model.edge = v;
            m_edge_slider.setCaption( "Edge: " + kvs::String::From( m_model.edge ) );
        } );
        m_edge_slider.sliderReleased( [&] ()
        {
            auto* scene = m_view.screen().scene();
            const std::string renderer_name = "Renderer" + kvs::String::From( index );
            for ( size_t index = 0; index < 2; ++index )
            {
                if ( m_model.ao )
                {
                    using Renderer = local::Model::AORenderer;
                    auto* renderer = Renderer::DownCast( scene->renderer( renderer_name ) );
                    renderer->setEdgeFactor( m_model.edge );
                }
            }
        } );

        // Opacity
        m_opacity_sliders[0].setCaption( "Opacity #0: " + kvs::String::From( m_model.opacities[0] ) );
        m_opacity_sliders[0].setValue( m_model.opacities[0] );
        m_opacity_sliders[0].setRange( 0, 1 );
        m_opacity_sliders[0].sliderMoved( [&] ()
        {
            m_model.opacities[0] = m_opacity_sliders[0].value();
            m_opacity_sliders[0].setCaption( "Opacity #0: " + kvs::String::From( m_model.opacities[0], 3 ) );
        } );
        m_opacity_sliders[0].sliderReleased( [&] ()
        {
            const auto* isosurface = m_view.screen().scene()->object( "Object0" );
            auto* object = new kvs::PolygonObject();
            object->shallowCopy( *kvs::PolygonObject::DownCast( isosurface ) );
            object->setOpacity( kvs::Math::Round( m_opacity_sliders[0].value() * 255 ) );
            m_view.screen().scene()->replaceObject( "Object0", object );
        } );

        m_opacity_sliders[1].setCaption( "Opacity #1: " + kvs::String::From( m_model.opacities[1] ) );
        m_opacity_sliders[1].setValue( m_model.opacities[1] );
        m_opacity_sliders[1].setRange( 0, 1 );
        m_opacity_sliders[1].sliderMoved( [&] ()
        {
            m_model.opacities[1] = m_opacity_sliders[1].value();
            m_opacity_sliders[1].setCaption( "Opacity #1: " + kvs::String::From( m_model.opacities[1], 3 ) );
        } );
        m_opacity_sliders[1].sliderReleased( [&] ()
        {
            const auto* isosurface = m_view.screen().scene()->object( "Object1" );
            auto* object = new kvs::PolygonObject();
            object->shallowCopy( *kvs::PolygonObject::DownCast( isosurface ) );
            object->setOpacity( kvs::Math::Round( m_opacity_sliders[1].value() * 255 ) );
            m_view.screen().scene()->replaceObject( "Object1", object );
        } );

        // Isovalue
        m_isovalue_sliders[0].setCaption( "Isovalue #0: " + kvs::String::From( isovalues[0] ) );
        m_isovalue_sliders[0].setValue( isovalues[0] );
        m_isovalue_sliders[0].setRange( min_values[0], max_values[0] );
        m_isovalue_sliders[0].sliderMoved( [&] ()
        {
            m_isovalue_sliders[0].setCaption( "Isovalue #1: " + kvs::String::From( m_isovalue_sliders[0].value() ) );
        } );
        m_isovalue_sliders[0].sliderReleased( [&] ()
        {
            auto* isosurface = m_model.isosurface( 0, m_isovalue_sliders[0].value() );;
            m_view.screen().scene()->replaceObject( "Object0", isosurface );
        } );

        m_isovalue_sliders[1].setCaption( "Isovalue #1: " + kvs::String::From( isovalues[0] ) );
        m_isovalue_sliders[1].setValue( isovalues[1] );
        m_isovalue_sliders[1].setRange( min_values[1], max_values[1] );
        m_isovalue_sliders[1].sliderMoved( [&] ()
        {
            m_isovalue_sliders[1].setCaption( "Isovalue #1: " + kvs::String::From( m_isovalue_sliders[1].value() ) );
        } );
        m_isovalue_sliders[1].sliderReleased( [&] ()
        {
            auto* isosurface = m_model.isosurface( 1, m_isovalue_sliders[1].value() );;
            m_view.screen().scene()->replaceObject( "Object1", isosurface );
        } );

        // Axis
        m_axis.setBoxType( kvs::OrientationAxis::SolidBox );

        // Colormap bar
        const auto cmap0 = m_model.tfuncs[0].colorMap();
        auto& cmap_bar0 = m_cmap_bars[0];
        cmap_bar0.setCaption( m_model.labels[0] );
        cmap_bar0.setColorMap( cmap0 );
        cmap_bar0.setRange( min_values[0], max_values[0] );

        const auto cmap1 = m_model.tfuncs[1].colorMap();
        auto& cmap_bar1 = m_cmap_bars[1];
        cmap_bar1.setCaption( m_model.labels[1] );
        cmap_bar1.setColorMap( cmap1 );
        cmap_bar1.setRange( min_values[1], max_values[1] );

        // Key press event
        m_key_event.update( [&] ( kvs::KeyEvent* event )
        {
            switch ( event->key() )
            {
            case kvs::Key::i:
            {
                this->setVisible( !this->isVisible(), false );
                break;
            }
            case kvs::Key::I:
            {
                this->setVisible( !this->isVisible(), true );
                break;
            }
            default:
                return;
            }
            m_view.redraw();
        } );

        // Paint event
        m_paint_event.update( [&] ()
        {
            static size_t counter = 1;
            static float timer = 0.0f;

            timer += m_view.compositor().timer().msec();
            if ( counter++ == 50 )
            {
                std::cout << "Rendering time: " << timer / counter << " [msec]" << std::endl;
                counter = 1;
                timer = 0.0f;
            }
        } );

        m_view.screen().addEvent( &m_capture_event );
        m_view.screen().addEvent( &m_target_change_event );
        m_view.screen().addEvent( &m_key_event );
        m_view.screen().addEvent( &m_paint_event );

        this->setLayout();
        this->setVisible();
    }

private:
    bool isVisible() const
    {
        return m_ao_check_box.isVisible();
    }

    void setVisible( const bool visible = true, const bool all = true )
    {
        m_ao_check_box.setVisible( visible );
        m_lod_check_box.setVisible( visible );
        m_repeat_slider.setVisible( visible );
        m_radius_slider.setVisible( visible );
        m_points_slider.setVisible( visible );
        m_edge_slider.setVisible( visible );
        m_opacity_sliders[0].setVisible( visible );
        m_opacity_sliders[1].setVisible( visible );
        m_isovalue_sliders[0].setVisible( visible );
        m_isovalue_sliders[1].setVisible( visible );

        if ( all )
        {
            m_axis.setVisible( visible );
            m_cmap_bars[0].setVisible( visible );
            m_cmap_bars[1].setVisible( visible );
        }
    }

    void setLayout( const int margin = 10 )
    {
        m_ao_check_box.setMargin( margin );
        m_ao_check_box.anchorToTopLeft();

        m_lod_check_box.setMargin( margin );
        m_lod_check_box.anchorToBottom( &m_ao_check_box );

        m_repeat_slider.setMargin( margin );
        m_repeat_slider.anchorToBottom( &m_lod_check_box );

        m_radius_slider.setMargin( margin );
        m_radius_slider.anchorToBottom( &m_repeat_slider );

        m_points_slider.setMargin( margin );
        m_points_slider.anchorToBottom( &m_radius_slider );

        m_edge_slider.setMargin( margin );
        m_edge_slider.anchorToBottom( &m_points_slider );

        m_opacity_sliders[0].setMargin( margin );
        m_opacity_sliders[0].anchorToTopRight();

        m_opacity_sliders[1].setMargin( margin );
        m_opacity_sliders[1].anchorToBottom( &m_opacity_sliders[0] );

        m_isovalue_sliders[0].setMargin( margin );
        m_isovalue_sliders[0].anchorToBottom( &m_opacity_sliders[1] );

        m_isovalue_sliders[1].setMargin( margin );
        m_isovalue_sliders[1].anchorToBottom( &m_isovalue_sliders[0] );

        m_axis.anchorToBottomLeft();

        auto& cmap_bar0 = m_cmap_bars[0];
        auto& cmap_bar1 = m_cmap_bars[1];
        cmap_bar0.anchorToBottomRight();
        cmap_bar1.anchorToTop( &cmap_bar0 );
    }
};

} // end of namespace local
