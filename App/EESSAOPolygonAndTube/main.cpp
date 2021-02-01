#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/TransferFunctionEditor>
#include <kvs/Slider>
#include <kvs/CheckBox>
#include <kvs/PolygonObject>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/KeyPressEventListener>
#include <kvs/TargetChangeEvent>
#include <kvs/ScreenCaptureEvent>
#include <iostream>
#include <kvs/DivergingColorMap>
#include "Streamline.h"
#include "SSAOStochasticPolygonRenderer.h"
#include "SSAOStochasticTubeRenderer.h"
#include "StochasticRenderingCompositor.h"
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/PaintEventListener>


kvs::Vec3i min_coord( 0, 0, 0 );
kvs::Vec3i max_coord( 250, 250, 250 );

kvs::PolygonObject* createIsosurface( std::string filename )
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
    kvs::StructuredVolumeObject* scalar_volume = new kvs::StructuredVectorToScalar( volume );

    double min_value = scalar_volume->minValue();
    double max_value = scalar_volume->maxValue();
    double isovalue = ( max_value + min_value ) * 0.05;  // Isolevel parameter.
    double opacity = 0.5;
    const kvs::PolygonObject::NormalType n = kvs::PolygonObject::VertexNormal;
    const bool d = false;
    const kvs::TransferFunction t( 256 );
    auto* polygon = new kvs::Isosurface( scalar_volume, isovalue, n, d, t );
    delete scalar_volume;
    polygon->setName( "Polygon" );
    polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );
    return polygon;
}

kvs::PointObject* generateSeedPoints( kvs::StructuredVolumeObject* volume, kvs::Vec3i stride )
{
    std::vector<kvs::Real32> v;
    for ( int k = volume->minObjectCoord().z(); k <= volume->maxObjectCoord().z(); k += stride.z() )
    {
        for ( int j = volume->minObjectCoord().y(); j <= volume->maxObjectCoord().y(); j += stride.y() )
        {
            for ( int i = volume->minObjectCoord().x(); i <= volume->maxObjectCoord().x(); i += stride.x() )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::PointObject* seeds = new kvs::PointObject;
    seeds->setCoords( kvs::ValueArray<kvs::Real32>( v ) );
    return seeds;
}

kvs::LineObject* createStreamline( std::string filename )
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
    kvs::ValueArray<float> values = volume->values().asValueArray<float>();
    for ( size_t i = 0; i < values.size(); i++ ) { values[i] *= 100.0; }
    volume->setValues( values );
    volume->updateMinMaxValues();
    kvs::Vec3i stride( 30, 30 ,30 );
    kvs::PointObject* seeds = generateSeedPoints( volume, stride );
    typedef local::Streamline Mapper;
    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( seeds );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    mapper->setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    return mapper->exec( volume );
}

/*===========================================================================*/
/**
 *  @brief  Main function. Tube and polygon visualization.
 *  @param  argc [i] argument count
 *  @param  argv [i] argument values
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::ShaderSource::AddSearchPath("../../../StochasticStreamline/Lib");

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle("SSAOStochasticRenderingCompositor Streamline and Polygon");
    screen.setSize( 1024, 1024 );
    screen.show();

    // Import volume object.
    std::string magnetic_file = argv[1];
    std::string velocity_file = argv[2];

    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( velocity_file );
    kvs::PolygonObject* polygon = createIsosurface( magnetic_file );
    kvs::LineObject* streamline = createStreamline( velocity_file );
    
    // Declare SSAOStochasticPolygonRenderer
    local::SSAOStochasticPolygonRenderer* polygon_renderer = new local::SSAOStochasticPolygonRenderer();
    polygon_renderer->setName( "PolygonRenderer" );
    polygon_renderer->setShader( kvs::Shader::BlinnPhong() );

    // Declare SSAOStochasticTubeRenderer
    local::SSAOStochasticTubeRenderer* tube_renderer = new local::SSAOStochasticTubeRenderer();
    tube_renderer->setName( "TubeRenderer" );
    tube_renderer->setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    tube_renderer->setShader( kvs::Shader::BlinnPhong() );
    
    // Register objects and renderers
    screen.registerObject( polygon, polygon_renderer );
    screen.registerObject( streamline, tube_renderer );

    // Declare SSAOStochasticRenderingCompositor.
    local::StochasticRenderingCompositor compositor( screen.scene() );
    compositor.setRepetitionLevel( 20 );
    compositor.enableLODControl();
    compositor.setShader( kvs::Shader::BlinnPhong() );
    screen.setEvent( &compositor );

    // Widgets.
    kvs::TransferFunctionEditor editor( &screen );
    editor.setPosition( screen.x() + screen.width(), screen.y() );
    editor.setVolumeObject( volume );
    editor.setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    editor.apply(
        [&]( kvs::TransferFunction tfunc ) {
            tube_renderer->setTransferFunction( tfunc );
            screen.redraw();
        } );
    editor.show();

    kvs::CheckBox checkbox( &screen );
    checkbox.setCaption( "LOD" );
    checkbox.setMargin( 10 );
    checkbox.setState( true );
    checkbox.anchorToTopLeft();
    checkbox.stateChanged(
        [&]() {
            compositor.setEnabledLODControl( checkbox.state() );
            screen.redraw();
        } );
    checkbox.show();

    kvs::Slider opacity( &screen );
    opacity.setCaption( "Opacity: " + kvs::String::ToString( 0.5 ) );
    opacity.setWidth( 150 );
    opacity.setMargin( 10 );
    opacity.setValue( 0.5 );
    opacity.setRange( 0, 1 );
    opacity.anchorToBottom( &checkbox );
    opacity.valueChanged(
        [&]() {
	  opacity.setCaption( "Opacity: " + kvs::String::ToString( opacity.value() ) );
            auto* scene = screen.scene();
            auto* object1 = kvs::PolygonObject::DownCast( scene->object( "Polygon" ) );
            auto* object2 = new kvs::PolygonObject();           
            object2->shallowCopy( *object1 );
            object2->setName( "Polygon" );
            object2->setOpacity( int( opacity.value() * 255 + 0.5 ) );
            scene->replaceObject( "Polygon", object2 );
        } );
    opacity.show();

    kvs::Slider repetition( &screen );
    repetition.setCaption( "Repetition" );
    repetition.setWidth( 150 );
    repetition.setMargin( 10 );
    repetition.setValue( 20 );
    repetition.setRange( 1, 100 );
    repetition.anchorToBottom( &opacity );
    repetition.valueChanged(
        [&]() {
            compositor.setRepetitionLevel( int( repetition.value() + 0.5 ) );
            screen.redraw();
        } );
    repetition.show();

    float radius = 0.5;
    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( radius ) );
    radius_slider.setValue( 0.5 );
    radius_slider.setRange( 0.01, 5.0 );
    radius_slider.setMargin( 10 );
    radius_slider.anchorToBottom( &repetition );
    radius_slider.show();
    radius_slider.sliderMoved( [&] ()
    {
        const float min_value = radius_slider.minValue();
        const float max_value = radius_slider.maxValue();
        const float v = int( radius_slider.value() * 10 ) * 0.1f;
        radius = kvs::Math::Clamp( v, min_value, max_value );
        radius_slider.setCaption( "Radius: " + kvs::String::From( radius ) );
    } );
    radius_slider.sliderReleased( [&] ()
    {
        compositor.setSamplingSphereRadius( radius );
        screen.redraw();
    } );

    int nsamples = 256;
    kvs::Slider nsample_slider( &screen );
    nsample_slider.setCaption( "Nsample: " + kvs::String::ToString( nsamples ) );
    nsample_slider.setValue( nsamples );
    nsample_slider.setRange( 1, 256 );
    nsample_slider.setMargin( 10 );
    nsample_slider.anchorToBottom( &radius_slider );
    nsample_slider.show();
    nsample_slider.sliderMoved( [&] ()
    {
        const int min_value = nsample_slider.minValue();
        const int max_value = nsample_slider.maxValue();
        const int v = int( nsample_slider.value() );
        nsamples = kvs::Math::Clamp( v, min_value, max_value );
        nsample_slider.setCaption( "Nsample: " + kvs::String::From( nsamples ) );
    } );
    nsample_slider.sliderReleased( [&] ()
    {
        compositor.setNumberOfSamplingPoints( nsamples );
        screen.redraw();
    } );

    float edge = 1.0;
    kvs::Slider edge_slider( &screen );
    edge_slider.setCaption( "Edge: " + kvs::String::ToString( edge ) );
    edge_slider.setValue( edge );
    edge_slider.setRange( 0.0, 5.0 );
    edge_slider.setMargin( 10 );
    edge_slider.anchorToBottom( &nsample_slider );
    edge_slider.show();
    edge_slider.sliderMoved( [&] ()
    {
        edge = int( edge_slider.value() * 10 ) * 0.1f;
        edge_slider.setCaption( "Edge: " + kvs::String::From( edge ) );
    } );
    edge_slider.sliderReleased( [&] ()
    {
      auto* scene = screen.scene();
      auto* renderer = local::SSAOStochasticPolygonRenderer::DownCast( scene->renderer( "PolygonRenderer" ) );
      renderer->setEdgeFactor( edge );
    } );

    kvs::KeyPressEventListener h_key;
    h_key.update( [&] ( kvs::KeyEvent* event )
    {
        switch( event->key() )
        {
        case kvs::Key::h:
        {
            if ( checkbox.isVisible() )
            {
                checkbox.hide();
                opacity.hide();
                repetition.hide();
                radius_slider.hide();
                nsample_slider.hide();
                edge_slider.hide();
            }
            else
            {
                checkbox.show();
                opacity.show();
                repetition.show();
                radius_slider.show();
                nsample_slider.show();
                edge_slider.show();
            }
        }
        default: break;
        }
    } );
    
    kvs::ScreenCaptureEvent capture_event;
    
    screen.addEvent( &h_key );
    screen.addEvent( &capture_event );

    kvs::PaintEventListener time;
    time.update( [&] ()
    {
        static size_t counter = 1;
        static float time = 0.0f;

        time += compositor.timer().msec();
        if ( counter++ == 10 )
        {
            std::cout << "Rendering time: " << time / counter << " [msec]" << std::endl;
            counter = 1;
            time = 0.0f;
        }

        } );
    
    screen.addEvent( &time );
                
    return app.run();
}
