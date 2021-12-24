#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/PolygonImporter>
#include <kvs/PolygonRenderer>
#include <kvs/LineObject>
#include <kvs/ShaderSource>
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
#include <kvs/PolygonToPolygon>
#include <kvs/StochasticPolygonRenderer>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>


/*===========================================================================*/
/**
 *  @brief  Model class manages SSAO parameters
 */
/*===========================================================================*/
struct Model
{
    using SSAORenderer = AmbientOcclusionRendering::SSAOStochasticPolygonRenderer;
    using Renderer = kvs::StochasticPolygonRenderer;

    // SSAO parameters
    bool ssao = true; ///< SSAO flag
    bool occlusion = false; ///< occlusion factor drawing flag
    float radius = 3.0f; ///< radius of point sampling region for SSAO
    int points = 256; ///< number of points used for SSAO
    float intensity = 2.0f; ///< SSAO intensity

    // Stochastic rendering parameters
    bool lod = true; ///< LoD flag
    size_t repeats = 50; ///< number of repetitions for stochasti rendering
    float opacity = 0.5f; ///< opacity of polygon object
    float edge = 0.0f; ///< edge intensity

    kvs::PolygonObject* import( const std::string filename )
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
        polygon->setName( "Object" );
        polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );
        return polygon;
    }

    kvs::RendererBase* renderer()
    {
        if ( ssao )
        {
            auto* renderer = new SSAORenderer();
            renderer->setName( "Renderer" );
            renderer->setRepetitionLevel( repeats );
            renderer->setLODControlEnabled( lod );
            renderer->setEdgeFactor( edge );
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
            renderer->setRepetitionLevel( repeats );
            renderer->setLODControlEnabled( lod );
            renderer->enableShading();
            return renderer;
        }
    }
};

/*===========================================================================*/
/**
 *  @brief  Main function
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle( "SSAOStochasticPolygonRenderer" );
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
    ssao_check_box.anchorToTopLeft();
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
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( model.radius ) );
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

    kvs::Slider edge_slider( &screen );
    edge_slider.setCaption( "Edge intensity: " + kvs::String::ToString( model.edge ) );
    edge_slider.setValue( model.edge );
    edge_slider.setRange( 0.0, 5.0 );
    edge_slider.setMargin( 10 );
    edge_slider.anchorToBottom( &points_slider );
    edge_slider.show();
    edge_slider.sliderMoved( [&] ()
    {
        float v = int( edge_slider.value() * 10 ) * 0.1f;
        model.edge = v;
        edge_slider.setCaption( "Edge intensity: " + kvs::String::From( model.edge ) );
    } );
    edge_slider.sliderReleased( [&] ()
    {
        screen.scene()->replaceRenderer( "Renderer", model.renderer() );
    } );

    kvs::Slider intensity_slider( &screen );
    intensity_slider.setCaption( "SSAO Intensity: " + kvs::String::From( model.intensity ) );
    intensity_slider.setValue( model.intensity );
    intensity_slider.setRange( 1, 10 );
    intensity_slider.setMargin( 10 );
    intensity_slider.anchorToBottom( &edge_slider );
    intensity_slider.show();
    intensity_slider.sliderMoved( [&] ()
    {
        model.intensity = int( intensity_slider.value() );
        intensity_slider.setCaption( "SSAO Intensity: " + kvs::String::From( model.intensity ) );
    } );
    intensity_slider.sliderReleased( [&] ()
    {
        if ( model.ssao )
        {
            screen.scene()->replaceRenderer( "Renderer", model.renderer() );
        }
    } );


    kvs::CheckBox lod_check_box( &screen );
    lod_check_box.setCaption( "LOD" );
    lod_check_box.setState( model.lod );
    lod_check_box.setMargin( 10 );
    lod_check_box.anchorToTopRight();
    lod_check_box.show();
    lod_check_box.stateChanged( [&] ()
    {
        model.lod = lod_check_box.state();
        auto* scene = screen.scene();
        if ( model.ssao )
        {
            auto* renderer = Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setLODControlEnabled( model.lod );
        }
        else
        {
            auto* renderer = Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
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
        if ( model.ssao )
        {
            auto* renderer = Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( model.repeats );
        }
        else
        {
            auto* renderer = Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( model.repeats );
        }
    } );

    kvs::Slider opacity_slider( &screen );
    opacity_slider.setCaption( "Opacity: " + kvs::String::From( model.opacity ) );
    opacity_slider.setValue( model.opacity );
    opacity_slider.setRange( 0, 1 );
    opacity_slider.setMargin( 10 );
    opacity_slider.anchorToBottom( &repeat_slider );
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
        object->setOpacity( kvs::Math::Round( model.opacity * 255 ) );
        screen.scene()->replaceObject( "Object", object );
    } );

/*
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

    kvs::CheckBox lod_check_box( &screen );
    lod_check_box.setCaption( "LOD" );
    lod_check_box.setState( model.lod );
    lod_check_box.setMargin( 10 );
    lod_check_box.anchorToBottom( &ssao_check_box );
    lod_check_box.show();
    lod_check_box.stateChanged( [&] ()
    {
        model.lod = lod_check_box.state();
        auto* scene = screen.scene();
        if ( model.ssao )
        {
            auto* renderer = Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setLODControlEnabled( model.lod );
        }
        else
        {
            auto* renderer = Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
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
        if ( model.ssao )
        {
            auto* renderer = Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( model.repeats );
        }
        else
        {
            auto* renderer = Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( model.repeats );
        }
    } );

    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( model.radius ) );
    radius_slider.setValue( model.radius );
    radius_slider.setRange( 0.1, 10.0 );
    radius_slider.setMargin( 10 );
    radius_slider.anchorToBottom( &repeat_slider );
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

    kvs::Slider opacity_slider( &screen );
    opacity_slider.setCaption( "Opacity: " + kvs::String::From( model.opacity ) );
    opacity_slider.setValue( model.opacity );
    opacity_slider.setRange( 0, 1 );
    opacity_slider.setMargin( 10 );
    opacity_slider.anchorToBottom( &points_slider );
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
        object->setOpacity( kvs::Math::Round( model.opacity * 255 ) );
        screen.scene()->replaceObject( "Object", object );
    } );

    kvs::Slider edge_slider( &screen );
    edge_slider.setCaption( "Edge intensity: " + kvs::String::ToString( model.edge ) );
    edge_slider.setValue( model.edge );
    edge_slider.setRange( 0.0, 5.0 );
    edge_slider.setMargin( 10 );
    edge_slider.anchorToBottom( &opacity_slider );
    edge_slider.show();
    edge_slider.sliderMoved( [&] ()
    {
        float v = int( edge_slider.value() * 10 ) * 0.1f;
        model.edge = v;
        edge_slider.setCaption( "Edge intensity: " + kvs::String::From( model.edge ) );
    } );
    edge_slider.sliderReleased( [&] ()
    {
        screen.scene()->replaceRenderer( "Renderer", model.renderer() );
    } );
*/


    // Events.
    kvs::KeyPressEventListener key_event( [&] ( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        case kvs::Key::i:
        {
            const bool visible = lod_check_box.isVisible();
            lod_check_box.setVisible( !visible );
//            const bool visible = ssao_check_box.isVisible();
//            ssao_check_box.setVisible( !visible );
//            lod_check_box.setVisible( !visible );
//            repeat_slider.setVisible( !visible );
//            radius_slider.setVisible( !visible );
//            points_slider.setVisible( !visible );
//            opacity_slider.setVisible( !visible );
//            edge_slider.setVisible( !visible );
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
