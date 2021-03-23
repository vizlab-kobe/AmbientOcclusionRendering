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
    kvs::Slider m_opacity_slider{ &m_view.screen() };
    kvs::Slider m_isovalue_slider{ &m_view.screen() };
    kvs::OrientationAxis m_axis{ &m_view.screen(), m_view.screen().scene() };
    kvs::ColorMapBar m_cmap_bars[2] = { kvs::ColorMapBar( &m_view.screen() ), kvs::ColorMapBar( &m_view.screen() ) };
    kvs::TransferFunctionEditor m_editor{ &m_view.screen() };

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
        auto* isosurface_volume = m_model.import( Input::MappingMethod::Isosurface );
        const auto isosurface_min_value = isosurface_volume->minValue();
        const auto isosurface_max_value = isosurface_volume->maxValue();
        const auto isovalue = ( isosurface_max_value + isosurface_min_value ) * 0.02f;

        // Initial values for streamline.
        auto* streamline_volume = m_model.import( Input::MappingMethod::Streamline );
        const auto streamline_min_value = streamline_volume->minValue();
        const auto streamline_max_value = streamline_volume->maxValue();

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
            // Isosurface renderer
            {
                using Renderer = local::Model::AOIsosurfaceRenderer;
                auto* renderer = Renderer::DownCast( scene->renderer( "IsosurfaceRenderer" ) );
                renderer->setEdgeFactor( m_model.edge );
            }
            // Streamline renderer
            {
                using Renderer = local::Model::AOStreamlineRenderer;
                auto* renderer = Renderer::DownCast( scene->renderer( "StreamlineRenderer" ) );
                renderer->setEdgeFactor( m_model.edge );
            }
        } );

        // Opacity
        m_opacity_slider.setCaption( "Opacity: " + kvs::String::From( m_model.opacity ) );
        m_opacity_slider.setValue( m_model.opacity );
        m_opacity_slider.setRange( 0, 1 );
        m_opacity_slider.sliderMoved( [&] ()
        {
            m_model.opacity = m_opacity_slider.value();
            m_opacity_slider.setCaption( "Opacity: " + kvs::String::From( m_model.opacity, 3 ) );
        } );
        m_opacity_slider.sliderReleased( [&] ()
        {
            const auto* isosurface = m_view.screen().scene()->object( "IsosurfaceObject" );
            auto* object = new kvs::PolygonObject();
            object->shallowCopy( *kvs::PolygonObject::DownCast( isosurface ) );
            object->setOpacity( kvs::Math::Round( m_opacity_slider.value() * 255 ) );
            m_view.screen().scene()->replaceObject( "IsosurfaceObject", object );
        } );

        // Isovalue
        m_isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( isovalue ) );
        m_isovalue_slider.setValue( isovalue );
        m_isovalue_slider.setRange( isosurface_min_value, isosurface_max_value );
        m_isovalue_slider.sliderMoved( [&] ()
        {
            m_isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( m_isovalue_slider.value() ) );
        } );
        m_isovalue_slider.sliderReleased( [&] ()
        {
            auto* isosurface = m_model.isosurface( m_isovalue_slider.value() );;
            m_view.screen().scene()->replaceObject( "IsosurfaceObject", isosurface );
        } );

        // Axis
        m_axis.setBoxType( kvs::OrientationAxis::SolidBox );

        // Colormap bar
        const auto isosurface_cmap = m_model.tfuncs[Model::MappingMethod::Isosurface].colorMap();
        auto& isosurface_cmap_bar = m_cmap_bars[Model::MappingMethod::Isosurface];
        isosurface_cmap_bar.setCaption( m_model.labels[Model::MappingMethod::Isosurface] );
        isosurface_cmap_bar.setColorMap( isosurface_cmap );
        isosurface_cmap_bar.setRange( isosurface_min_value, isosurface_max_value );

        const auto streamline_cmap = m_model.tfuncs[Model::MappingMethod::Streamline].colorMap();
        auto& streamline_cmap_bar = m_cmap_bars[Model::MappingMethod::Streamline];
        streamline_cmap_bar.setCaption( m_model.labels[Model::MappingMethod::Streamline] );
        streamline_cmap_bar.setColorMap( streamline_cmap );
        streamline_cmap_bar.setRange( streamline_min_value, streamline_max_value );

        // Transfer function editor
        m_editor.setTransferFunction( m_model.tfuncs[Model::MappingMethod::Streamline] );
        m_editor.setVolumeObject( m_model.import( Model::MappingMethod::Streamline ) );
        m_editor.apply( [&] ( kvs::TransferFunction tfunc )
        {
            m_model.tfuncs[Model::MappingMethod::Streamline] = tfunc;

            auto* scene = m_view.screen().scene();
            using Renderer = local::Model::AOStreamlineRenderer;
            auto* renderer = Renderer::DownCast( scene->renderer( "StreamlineRenderer" ) );
            renderer->setTransferFunction( tfunc );

            auto& streamline_cmap_bar = m_cmap_bars[Model::MappingMethod::Streamline];
            streamline_cmap_bar.setColorMap( tfunc.colorMap() );
            m_view.screen().redraw();
        } );
        m_editor.show();

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
        m_opacity_slider.setVisible( visible );
        m_isovalue_slider.setVisible( visible );
        m_edge_slider.setVisible( visible );

        if ( all )
        {
            m_axis.setVisible( visible );
            m_cmap_bars[Model::MappingMethod::Isosurface].setVisible( visible );
            m_cmap_bars[Model::MappingMethod::Streamline].setVisible( visible );
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

        m_opacity_slider.setMargin( margin );
        m_opacity_slider.anchorToTopRight();

        m_isovalue_slider.setMargin( margin );
        m_isovalue_slider.anchorToBottom( &m_opacity_slider );

        m_axis.anchorToBottomLeft();

        auto& isosurface_cmap_bar = m_cmap_bars[Model::MappingMethod::Isosurface];
        auto& streamline_cmap_bar = m_cmap_bars[Model::MappingMethod::Streamline];
        streamline_cmap_bar.anchorToBottomRight();
        isosurface_cmap_bar.anchorToTop( &streamline_cmap_bar );
    }
};

} // end of namespace local
