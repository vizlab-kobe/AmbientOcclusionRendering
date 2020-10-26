#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVolumeObject>
#include <kvs/PolygonRenderer>
#include <kvs/ShaderSource>
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
#include <kvs/PolygonToPolygon>
#include <kvs/StochasticPolygonRenderer>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>


/*===========================================================================*/
/**
 *  @brief  Model class manages SSAO parameters
 */
/*===========================================================================*/
struct Model
{
    using SSAORenderer = AmbientOcclusionRendering::SSAOStochasticPolygonRenderer;
    using Renderer = kvs::StochasticPolygonRenderer;

    bool ssao; ///< SSAO flag
    bool lod; ///< LoD flag
    size_t repeats; ///< number of repetitions for stochasti rendering
    float radius; ///< radius of point sampling region for SSAO
    int points; ///< number of points used for SSAO
    float opacity; ///< opacity of polygon object
    double minValue;
    double maxValue;
    double isovalue;

    kvs::PolygonObject* import( const std::string filename )
    {
        kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
        kvs::StructuredVolumeObject* scalar = new kvs::StructuredVectorToScalar( volume );

        minValue = scalar->minValue();
        maxValue = scalar->maxValue();
        isovalue = ( maxValue + minValue ) * 0.5;
        const kvs::PolygonObject::NormalType n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
        const kvs::TransferFunction t( 256 );
        auto* polygon = new kvs::Isosurface( scalar, isovalue, n, d, t );
        
        delete scalar;
        polygon->setName( "Object" );
        polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );
        return polygon;
    }

    kvs::PolygonObject* changeIsovalue( const std::string filename )
    {
        kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
        kvs::StructuredVolumeObject* scalar = new kvs::StructuredVectorToScalar( volume );

        const kvs::PolygonObject::NormalType n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
        const kvs::TransferFunction t( 256 );
        auto* polygon = new kvs::Isosurface( scalar, isovalue, n, d, t );
        
        delete scalar;
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
            renderer->setEnabledLODControl( lod );
            renderer->setSamplingSphereRadius( radius );
            renderer->setNumberOfSamplingPoints( points );
            renderer->enableShading();
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "Renderer" );
            renderer->setRepetitionLevel( repeats );
            renderer->setEnabledLODControl( lod );
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
    kvs::ShaderSource::AddSearchPath( "../../Lib" );
    kvs::ShaderSource::AddSearchPath( "../../../StochasticStreamline/Lib" );

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle( "MagneticField" );
    screen.show();

    // Parameters.
    Model model;
    model.ssao = true;
    model.lod = true;
    model.repeats = 20;
    model.radius = 0.5;
    model.points = 256;
    model.opacity = 0.5f;

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
            renderer->setEnabledLODControl( model.lod );
        }
        else
        {
            auto* renderer = Model::Renderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( model.lod );
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
    radius_slider.setRange( 0.1, 5.0 );
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

    kvs::Slider isovalue_slider( &screen );
    isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( model.isovalue ) );
    isovalue_slider.setValue( model.isovalue );
    isovalue_slider.setRange( model.minValue, model.maxValue );
    isovalue_slider.setMargin( 10 );
    isovalue_slider.anchorToBottom( &opacity_slider );
    isovalue_slider.show();
    isovalue_slider.sliderMoved( [&] ()
    {
        model.isovalue = isovalue_slider.value();
        isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( model.isovalue) );
    } );
    isovalue_slider.sliderReleased( [&] ()
    {
        screen.scene()->replaceObject( "Object", model.changeIsovalue( filename ) );
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
            lod_check_box.setVisible( !visible );
            repeat_slider.setVisible( !visible );
            radius_slider.setVisible( !visible );
            points_slider.setVisible( !visible );
            opacity_slider.setVisible( !visible );
            isovalue_slider.setVisible( !visible );
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
