#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/ShaderSource>
#include <kvs/PolygonImporter>
#include <kvs/PolygonRenderer>
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
#include <kvs/PolygonToPolygon>
#include <AmbientOcclusionRendering/Lib/SSAOPolygonRenderer.h>


namespace
{

inline kvs::PolygonObject* ImportPolygonObject( const std::string filename )
{
    kvs::PolygonObject* polygon = new kvs::PolygonImporter( filename );
    const size_t nvertices = polygon->numberOfVertices();
    const size_t npolygons = polygon->numberOfConnections();
    if ( npolygons > 0 && nvertices != 3 * npolygons )
    {
        kvs::PolygonObject* temp = new kvs::PolygonToPolygon( polygon );
        delete polygon;
        polygon = temp;
        }
    return polygon;
}

inline kvs::RendererBase* CreateRenderer( const bool ssao, const float radius, const size_t points )
{
    if ( ssao )
    {
        auto* renderer = new AmbientOcclusionRendering::SSAOPolygonRenderer();
        renderer->setName( "Renderer" );
        renderer->setSamplingSphereRadius( radius );
        renderer->setNumberOfSamplingPoints( points );
        renderer->enableShading();
        return renderer;
    }
    else
    {
        auto* renderer = new kvs::glsl::PolygonRenderer();
        renderer->setName( "Renderer" );
        renderer->enableShading();
        return renderer;
    }
}

} // end of namespace


int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setTitle( "Screen Space Ambient Occlusion" );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.show();

    // Parameters.
    bool ssao = true;
    float radius = 0.5f;
    size_t points = 256;

    // Visualization pipeline.
    const std::string filename = argv[1];
    auto* object = ::ImportPolygonObject( filename );
    auto* renderer = ::CreateRenderer( ssao, radius, points );
    screen.registerObject( object, renderer );

    // Widgets.
    kvs::CheckBox ssao_check_box( &screen );
    ssao_check_box.setCaption( "SSAO" );
    ssao_check_box.setState( ssao );
    ssao_check_box.setMargin( 10 );
    ssao_check_box.stateChanged(
        [&] ()
        {
            ssao = ssao_check_box.state();
            renderer = ::CreateRenderer( ssao, radius, points );
            screen.scene()->replaceRenderer( "Renderer", renderer );
        } );
    ssao_check_box.show();

    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::From( radius ) );
    radius_slider.setValue( radius );
    radius_slider.setRange( 0.1, 5.0 );
    radius_slider.setMargin( 10 );
    radius_slider.anchorToBottom( &ssao_check_box );
    radius_slider.sliderMoved(
        [&] ()
        {
            const float min_value = radius_slider.minValue();
            const float max_value = radius_slider.maxValue();
            const float v = int( radius_slider.value() * 2 ) * 0.5f;
            radius = kvs::Math::Clamp( v, min_value, max_value );
            radius_slider.setCaption( "Radius: " + kvs::String::From( radius ) );
        } );
    radius_slider.sliderReleased(
        [&] ()
        {
            if ( ssao )
            {
                renderer = ::CreateRenderer( ssao, radius, points );
                screen.scene()->replaceRenderer( "Renderer", renderer );
            }
        } );
    radius_slider.show();

    kvs::Slider points_slider( &screen );
    points_slider.setCaption( "Points: " + kvs::String::From( points ) );
    points_slider.setValue( points );
    points_slider.setRange( 1, 256 );
    points_slider.setMargin( 10 );
    points_slider.anchorToBottom( &radius_slider );
    points_slider.sliderMoved(
        [&] ()
        {
            points = int( points_slider.value() );
            points_slider.setCaption( "Points: " + kvs::String::From( points ) );
        } );
    points_slider.sliderReleased(
        [&] ()
        {
            if ( ssao )
            {
                renderer = ::CreateRenderer( ssao, radius, points );
                screen.scene()->replaceRenderer( "Renderer", renderer );
            }
        } );
    points_slider.show();

    // Events.
    kvs::KeyPressEventListener key_event(
        [&] ( kvs::KeyEvent* event )
        {
            switch ( event->key() )
            {
            case kvs::Key::i:
            {
                const bool visible = ssao_check_box.isVisible();
                ssao_check_box.setVisible( !visible );
                radius_slider.setVisible( !visible );
                points_slider.setVisible( !visible );
                screen.redraw();
                break;
            }
            default: break;
            }
        } );
    screen.addEvent( &key_event );

    kvs::ScreenCaptureEvent capture_event;
    screen.addEvent( &capture_event );

    kvs::TargetChangeEvent target_change_event;
    screen.addEvent( &target_change_event );

    return app.run();
}
