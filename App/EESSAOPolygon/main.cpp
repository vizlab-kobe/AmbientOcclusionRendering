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
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/RadioButton>
#include <kvs/RadioButtonGroup>
#include <kvs/PaintEventListener>
#include <kvs/ColorMapBar>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>


/*===========================================================================*/
/**
 *  @brief  Model class manages SSAO parameters
 */
/*===========================================================================*/
struct Model
{
    using AORenderer = AmbientOcclusionRendering::SSAOStochasticPolygonRenderer;
    using Renderer = kvs::StochasticPolygonRenderer;

    bool ao; ///< AO flag
    bool lod; ///< LoD flag
    size_t repeats; ///< number of repetitions for stochasti rendering
    float radius; ///< radius of point sampling region for SSAO
    int points; ///< number of points used for SSAO
    float opacity; ///< opacity of polygon object
    float edge; ///< edge factor
    double min_value; ///< min. value
    double max_value; ///< max. value
    double isovalue; ///< isovalue for isosurface extraction

    kvs::PolygonObject* import( const std::string filename )
    {
        auto* volume = new kvs::StructuredVolumeImporter( filename );
        auto* scalar = new kvs::StructuredVectorToScalar( volume );

        min_value = scalar->minValue();
        max_value = scalar->maxValue();
        isovalue = ( max_value + min_value ) * 0.02;

        const auto n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
//        const kvs::TransferFunction t( 256 );
        const auto t = this->transferFunction();
        auto* polygon = new kvs::Isosurface( scalar, isovalue, n, d, t );
        polygon->setName( "Object" );
        polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );

        delete scalar;
        return polygon;
    }

    kvs::PolygonObject* changeIsovalue( const std::string filename )
    {
        auto* volume = new kvs::StructuredVolumeImporter( filename );
        auto* scalar = new kvs::StructuredVectorToScalar( volume );

        const auto n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
//        const kvs::TransferFunction t( 256 );
        const auto t = this->transferFunction();
        auto* polygon = new kvs::Isosurface( scalar, isovalue, n, d, t );

        delete scalar;
        polygon->setName( "Object" );
        polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );
        return polygon;
    }

    kvs::RendererBase* renderer()
    {
        if ( this->ao )
        {
            auto* renderer = new AORenderer();
            renderer->setName( "Renderer" );
            renderer->setRepetitionLevel( repeats );
            renderer->setLODControlEnabled( lod );
            renderer->setSamplingSphereRadius( radius );
            renderer->setNumberOfSamplingPoints( points );
            renderer->setEdgeFactor( edge );
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

    kvs::TransferFunction transferFunction()
    {
        //const auto cmap = kvs::ColorMap::CoolWarm( 256 );
        const auto cmap = kvs::ColorMap::BrewerSpectral( 256 );
        //const auto cmap = kvs::ColorMap::BrewerRdBu( 256 );
        //const auto cmap = kvs::ColorMap::BrewerRdYlGn( 256 );
        //const auto cmap = kvs::ColorMap::Viridis( 256 );
        return kvs::TransferFunction( cmap );
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

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle( "MagneticField" );
    screen.show();

    // Parameters.
    Model model;
    model.ao = true;
    model.lod = true;
    model.repeats = 20;
    model.radius = 0.5;
    model.points = 256;
    model.opacity = 0.1f;
    model.edge = 1.0;

    // Visualization pipeline.
    const std::string filename = argv[1];
    screen.registerObject( model.import( filename ), model.renderer() );

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
            auto* renderer = Model::AORenderer::DownCast( scene->renderer( "Renderer" ) );
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
        if ( model.ao )
        {
            auto* renderer = Model::AORenderer::DownCast( scene->renderer( "Renderer" ) );
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
    isovalue_slider.setCaption( "Isovalue: " + kvs::String::From( model.isovalue ) );
    isovalue_slider.setValue( model.isovalue );
    isovalue_slider.setRange( model.min_value, model.max_value );
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

    const auto cmap = model.transferFunction().colorMap();
    kvs::ColorMapBar cmap_bar( &screen );
    cmap_bar.setCaption( " " );
    cmap_bar.setColorMap( cmap );
    cmap_bar.anchorToBottomRight();
    cmap_bar.setRange( model.min_value, model.max_value );
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
        static float time = 0.0f;

        time += screen.scene()->renderer("Renderer")->timer().msec();
        if ( counter++ == 50 )
        {
            std::cout << "Rendering time: " << time / counter << " [msec]" << std::endl;
            counter = 1;
            time = 0.0f;
        }
    } );

    screen.addEvent( &time );

    return app.run();
}
