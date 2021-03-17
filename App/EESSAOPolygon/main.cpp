#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVolumeObject>
#include <kvs/ShaderSource>
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
#include <kvs/RadioButton>
#include <kvs/RadioButtonGroup>
#include <kvs/PaintEventListener>
#include <kvs/ColorMapBar>

#include "Input.h"
#include "Model.h"

/*===========================================================================*/
/**
 *  @brief  Main function
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath( "../../Lib" );

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle( "MagneticField" );
    screen.show();

    // Input parameters.
    local::Input input;
    if ( !input.parse( argc, argv ) ) { return 1; }

    local::Model model( input );

    // Isosurface extraction.
    auto* volume = model.import();
    const auto min_value = volume->minValue();
    const auto max_value = volume->maxValue();
    const auto isovalue = ( max_value + min_value ) * 0.02f;
    screen.registerObject( model.isosurface( isovalue ), model.renderer() );

    // Widgets.
    kvs::CheckBox ao_check_box( &screen );
    ao_check_box.setCaption( "AO" );
    ao_check_box.setState( model.lod );
    ao_check_box.setMargin( 10 );
    ao_check_box.anchorToTopLeft();
    ao_check_box.show();
    ao_check_box.stateChanged( [&] ()
    {
        model.ao = ao_check_box.state();
        screen.scene()->replaceRenderer( "Renderer", model.renderer() );
    } );

    kvs::CheckBox lod_check_box( &screen );
    lod_check_box.setCaption( "LOD" );
    lod_check_box.setState( model.lod );
    lod_check_box.setMargin( 10 );
    lod_check_box.anchorToBottom( &ao_check_box );
    lod_check_box.show();
    lod_check_box.stateChanged( [&] ()
    {
        model.lod = lod_check_box.state();
        auto* scene = screen.scene();
        if ( model.ao )
        {
            auto* renderer = local::Model::AORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setLODControlEnabled( model.lod );
        }
        else
        {
            auto* renderer = local::Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setLODControlEnabled( model.lod );
        }
    } );

    kvs::Slider repeat_slider( &screen );
    repeat_slider.setCaption( "Repeats: " + kvs::String::ToString( model.repeats ) );
    repeat_slider.setValue( model.repeats );
    repeat_slider.setRange( 1, 100 );
    repeat_slider.setMargin( 10 );
    repeat_slider.anchorToBottom( &lod_check_box );
    repeat_slider.show();
    repeat_slider.sliderMoved( [&] ()
    {
        model.repeats = repeat_slider.value();
        repeat_slider.setCaption( "Repeats: " + kvs::String::From( model.repeats ) );
    } );
    repeat_slider.sliderReleased( [&] ()
    {
        auto* scene = screen.scene();
        if ( model.ao )
        {
            auto* renderer = local::Model::AORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( model.repeats );
        }
        else
        {
            auto* renderer = local::Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( model.repeats );
        }
    } );

    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( model.radius ) );
    radius_slider.setValue( model.radius );
    radius_slider.setRange( 0.1, 5.0 );
    radius_slider.setMargin( 10 );
    radius_slider.anchorToBottom( &repeat_slider );
    radius_slider.show();
    radius_slider.sliderMoved( [&] ()
    {
        const float min_rad = radius_slider.minValue();
        const float max_rad = radius_slider.maxValue();
        const float v = int( radius_slider.value() * 2 ) * 0.5f;
        model.radius = kvs::Math::Clamp( v, min_rad, max_rad );
        radius_slider.setCaption( "Radius: " + kvs::String::From( model.radius ) );
    } );
    radius_slider.sliderReleased( [&] ()
    {
        if ( model.ao )
        {
            screen.scene()->replaceRenderer( "Renderer", model.renderer() );
        }
    } );

    kvs::Slider points_slider( &screen );
    points_slider.setCaption( "Points: " + kvs::String::From( model.points ) );
    points_slider.setValue( model.points );
    points_slider.setRange( 1, 256 );
    points_slider.setMargin( 10 );
    points_slider.anchorToBottom( &radius_slider );
    points_slider.show();
    points_slider.sliderMoved( [&] ()
    {
        model.points = int( points_slider.value() );
        points_slider.setCaption( "Points: " + kvs::String::From( model.points ) );
    } );
    points_slider.sliderReleased( [&] ()
    {
        if ( model.ao )
        {
            screen.scene()->replaceRenderer( "Renderer", model.renderer() );
        }
    } );

    kvs::Slider edge_slider( &screen );
    edge_slider.setCaption( "Edge: " + kvs::String::ToString( model.edge ) );
    edge_slider.setValue( model.edge );
    edge_slider.setRange( 0.0, 5.0 );
    edge_slider.setMargin( 10 );
    edge_slider.anchorToBottom( &points_slider );
    edge_slider.show();
    edge_slider.sliderMoved( [&] ()
    {
        float v = int( edge_slider.value() * 10 ) * 0.1f;
        model.edge = v;
        edge_slider.setCaption( "Edge: " + kvs::String::From( model.edge ) );
    } );
    edge_slider.sliderReleased( [&] ()
    {
        screen.scene()->replaceRenderer( "Renderer", model.renderer() );
    } );

    kvs::Slider opacity_slider( &screen );
    opacity_slider.setCaption( "Opacity: " + kvs::String::From( model.opacity ) );
    opacity_slider.setValue( model.opacity );
    opacity_slider.setRange( 0, 1 );
    opacity_slider.setMargin( 10 );
    opacity_slider.anchorToTopRight();
    opacity_slider.show();
    opacity_slider.sliderMoved( [&] ()
    {
        model.opacity = opacity_slider.value();
        opacity_slider.setCaption( "Opacity: " + kvs::String::From( model.opacity, 3 ) );
    } );
    opacity_slider.sliderReleased( [&] ()
    {
        auto* object = new kvs::PolygonObject();
        object->shallowCopy( *kvs::PolygonObject::DownCast( screen.scene()->object( "Object" ) ) );
        object->setOpacity( kvs::Math::Round( opacity_slider.value() * 255 ) );
        screen.scene()->replaceObject( "Object", object );
    } );

    kvs::Slider isovalue_slider( &screen );
//    isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( model.isovalue ) );
    isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( isovalue ) );
//    isovalue_slider.setValue( model.isovalue );
//    isovalue_slider.setRange( model.min_value, model.max_value );
    isovalue_slider.setValue( isovalue );
    isovalue_slider.setRange( min_value, max_value );
    isovalue_slider.setMargin( 10 );
    isovalue_slider.anchorToBottom( &opacity_slider );
    isovalue_slider.show();
    isovalue_slider.sliderMoved( [&] ()
    {
//        model.isovalue = isovalue_slider.value();
//        isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( model.isovalue) );
        isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( isovalue_slider.value() ) );
    } );
    isovalue_slider.sliderReleased( [&] ()
    {
//        screen.scene()->replaceObject( "Object", model.changeIsovalue( filename ) );
        screen.scene()->replaceObject( "Object", model.isosurface( isovalue_slider.value() ) );
    } );

    const auto cmap = model.transferFunction().colorMap();
    kvs::ColorMapBar cmap_bar( &screen );
    cmap_bar.setCaption( " " );
    cmap_bar.setColorMap( cmap );
    cmap_bar.anchorToBottomRight();
//    cmap_bar.setRange( model.min_value, model.max_value );
    cmap_bar.setRange( min_value, max_value );
    cmap_bar.show();

    // Events.
    kvs::KeyPressEventListener key_event( [&] ( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        case kvs::Key::i:
        {
            const bool visible = lod_check_box.isVisible();
            ao_check_box.setVisible( !visible );
            lod_check_box.setVisible( !visible );
            repeat_slider.setVisible( !visible );
            radius_slider.setVisible( !visible );
            points_slider.setVisible( !visible );
            opacity_slider.setVisible( !visible );
            isovalue_slider.setVisible( !visible );
            edge_slider.setVisible( !visible );
            screen.redraw();
            break;
        }
        default:
            break;
        }
    } );
    screen.addEvent( &key_event );

    kvs::ScreenCaptureEvent capture_event;
    screen.addEvent( &capture_event );

    kvs::TargetChangeEvent target_change_event;
    screen.addEvent( &target_change_event );

    // Measure time. 10 times average.
    kvs::PaintEventListener time;
    time.update( [&] ()
    {
        static size_t counter = 1;
        static float timer = 0.0f;

        timer += screen.scene()->renderer("Renderer")->timer().msec();
        if ( counter++ == 50 )
        {
            std::cout << "Rendering time: " << timer / counter << " [msec]" << std::endl;
            counter = 1;
            timer = 0.0f;
        }
    } );

    screen.addEvent( &time );

    return app.run();
}
