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


/*===========================================================================*/
/**
 *  @brief  Model class manages SSAO parameters
 */
/*===========================================================================*/
struct Model
{
    using SSAORenderer = AmbientOcclusionRendering::SSAOPolygonRenderer;
    using Renderer = kvs::glsl::PolygonRenderer;

    bool ssao = true; ///< SSAO flag
    bool occlusion = false; ///< occlusion factor drawing flag
    float radius = 3.0f; ///< radius of point sampling region for SSAM
    size_t points = 256; ///< number of points used for SSAO
    float intensity = 2.0f; ///< SSAO intensity

    kvs::PolygonObject* import( const std::string filename ) const
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

    kvs::RendererBase* renderer() const
    {
        if ( ssao )
        {
            auto* renderer = new SSAORenderer();
            renderer->setName( "Renderer" );
            renderer->aoBuffer().setKernelRadius( radius );
            renderer->aoBuffer().setKernelSize( points );
            renderer->aoBuffer().setIntensity( intensity );
            renderer->aoBuffer().setDrawingOcclusionFactorEnabled( occlusion );
            renderer->enableShading();
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "Renderer" );
            renderer->enableShading();
            return renderer;
        }
    }
};

/*===========================================================================*/
/**
 *  @brief  Main function.
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setTitle( "SSAOPolygonRenderer" );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.show();

    // Parameters.
    Model model;

    // Visualization pipeline.
    const std::string filename = argv[1];
    screen.registerObject( model.import( filename ), model.renderer() );

    // Widgets.
    kvs::CheckBox ssao_check_box( &screen );
    ssao_check_box.setCaption( "SSAO" );
    ssao_check_box.setState( model.ssao );
    ssao_check_box.setMargin( 10 );
    ssao_check_box.show();
    ssao_check_box.stateChanged( [&] ()
    {
        model.ssao = ssao_check_box.state();
        screen.scene()->replaceRenderer( "Renderer", model.renderer() );
    } );

    kvs::CheckBox occlusion_check_box( &screen );
    occlusion_check_box.setCaption( "Occlusion factor" );
    occlusion_check_box.setState( model.occlusion );
    occlusion_check_box.setMargin( 10 );
    occlusion_check_box.anchorToBottom( &ssao_check_box );
    occlusion_check_box.show();
    occlusion_check_box.stateChanged( [&] ()
    {
        model.occlusion = occlusion_check_box.state();
        screen.scene()->replaceRenderer( "Renderer", model.renderer() );
    } );

    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::From( model.radius ) );
    radius_slider.setValue( model.radius );
    radius_slider.setRange( 0.1, 10.0 );
    radius_slider.setMargin( 10 );
    radius_slider.anchorToBottom( &occlusion_check_box );
    radius_slider.show();
    radius_slider.sliderMoved( [&] ()
    {
        const float min_value = radius_slider.minValue();
        const float max_value = radius_slider.maxValue();
        const float v = int( radius_slider.value() * 2 ) * 0.5f;
        model.radius = kvs::Math::Clamp( v, min_value, max_value );
        radius_slider.setCaption( "Radius: " + kvs::String::From( model.radius ) );
    } );
    radius_slider.sliderReleased( [&] ()
    {
        if ( model.ssao )
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
        if ( model.ssao )
        {
            screen.scene()->replaceRenderer( "Renderer", model.renderer() );
        }
    } );

    kvs::Slider intensity_slider( &screen );
    intensity_slider.setCaption( "Intensity: " + kvs::String::From( model.intensity ) );
    intensity_slider.setValue( model.intensity );
    intensity_slider.setRange( 1, 10 );
    intensity_slider.setMargin( 10 );
    intensity_slider.anchorToBottom( &points_slider );
    intensity_slider.show();
    intensity_slider.sliderMoved( [&] ()
    {
        model.intensity = int( intensity_slider.value() );
        intensity_slider.setCaption( "Intensity: " + kvs::String::From( model.intensity ) );
    } );
    intensity_slider.sliderReleased( [&] ()
    {
        if ( model.ssao )
        {
            screen.scene()->replaceRenderer( "Renderer", model.renderer() );
        }
    } );

    // Events.
    kvs::KeyPressEventListener key_event( [&] ( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        case kvs::Key::i:
        {
            const bool visible = ssao_check_box.isVisible();
            ssao_check_box.setVisible( !visible );
            radius_slider.setVisible( !visible );
            points_slider.setVisible( !visible );
            intensity_slider.setVisible( !visible );
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
