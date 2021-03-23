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
#include "SSAOStochasticPolygonRenderer.h"
#include "StochasticRenderingCompositor.h"
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/PaintEventListener>
#include <kvs/ColorMap>
#include <kvs/Xform>
#include <kvs/OrientationAxis>
#include <cmath>


/*===========================================================================*/
/**
 *  @brief  Model class manages SSAO parameters
 */
/*===========================================================================*/
struct Model
{
    float edge;
    double min;
    double max;
    double isolevel;
    double opacity;

    kvs::PolygonObject* import( const std::string& filename )
    {
       kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
       kvs::StructuredVolumeObject* scalar = new kvs::StructuredVectorToScalar( volume );

       min = scalar->minValue();
       max = scalar->maxValue();
       const double isovalue = ( max + min ) * isolevel;

       const kvs::PolygonObject::NormalType n = kvs::PolygonObject::VertexNormal;
       const bool d = false;
       const kvs::TransferFunction t( 256 );
       auto* polygon = new kvs::Isosurface( scalar, isovalue, n, d, t );
       polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );

       delete scalar;
       return polygon;
    }
};
    
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
    screen.setTitle("SSAO polygon and polygon");
//    screen.setSize( 1024, 1024 );
    screen.show();

    Model magnetic;
    magnetic.edge = 1.0;
    magnetic.opacity = 0.5;
    magnetic.isolevel = 0.05;
    
    Model velocity;
    velocity.edge = 1.0;
    velocity.opacity = 0.5;
    velocity.isolevel = 0.55;
    
    // Import volume object.
    std::string magnetic_file = argv[1];
    std::string velocity_file = argv[2];

    float pi = 3.14159;
    kvs::Xform xform();
    kvs::Mat3 rotatex( 1.0,                0,                 0,
                         0, cosf( -0.33*pi ), -sinf( -0.33*pi ),
                         0, sinf( -0.33*pi ),  cosf( -0.33*pi ) );
    
    kvs::Mat3 rotatey(  cosf( 0.33*pi ),   0, sinf( 0.33*pi ),
                                      0, 1.0,               0,
                       -sinf( 0.33*pi ),   0, cosf( 0.33*pi ) );
    
    kvs::Mat3 rotatez( cosf( 0.33*pi ), -sinf( 0.33*pi ),    0,
                       sinf( 0.33*pi ),  cosf( 0.33*pi ),    0,
                                     0,                0,  1.0 );
     //xform.Rotation(

    auto* mag_polygon = magnetic.import( magnetic_file );
    mag_polygon->setName( "Magnetic" );
    mag_polygon->XformControl::rotate( rotatex );
    mag_polygon->XformControl::rotate( rotatey );
    auto* vel_polygon = velocity.import( velocity_file );
    //auto* vel_polygon = velocity.import( magnetic_file );
    vel_polygon->setName( "Velocity" );
    vel_polygon->XformControl::rotate( rotatex );
    vel_polygon->XformControl::rotate( rotatey );
    
    // Declare SSAOStochasticPolygonRenderer
    auto* mag_renderer = new local::SSAOStochasticPolygonRenderer();
    mag_renderer->setName( "MagneticRenderer" );

    auto* vel_renderer = new local::SSAOStochasticPolygonRenderer();
    vel_renderer->setName( "VelocityRenderer" );
    
    // Register objects and renderers
    screen.registerObject( mag_polygon, mag_renderer );
    screen.registerObject( vel_polygon, vel_renderer );

    // Declare SSAOStochasticRenderingCompositor.
    local::StochasticRenderingCompositor compositor( screen.scene() );
    compositor.setRepetitionLevel( 20 );
    compositor.enableLODControl();
    compositor.setShader( kvs::Shader::BlinnPhong() );
    screen.setEvent( &compositor );

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
    opacity.setCaption( "Magnetic Opacity: " + kvs::String::ToString( magnetic.opacity ) );
    opacity.setWidth( 150 );
    opacity.setMargin( 10 );
    opacity.setValue( magnetic.opacity );
    opacity.setRange( 0, 1 );
    opacity.anchorToBottom( &checkbox );
    opacity.valueChanged(
        [&]() {
            magnetic.opacity = opacity.value();
            opacity.setCaption( "Magnetic opacity: " + kvs::String::ToString( magnetic.opacity ) );
            auto* scene = screen.scene();
            auto* object1 = kvs::PolygonObject::DownCast( scene->object( "Magnetic" ) );
            auto* object2 = new kvs::PolygonObject();           
            object2->shallowCopy( *object1 );
            object2->setName( "Magnetic" );
            object2->setOpacity( int( magnetic.opacity * 255 + 0.5 ) );
            scene->replaceObject( "Magnetic", object2 );
        } );
    opacity.show();

    kvs::Slider opacity2( &screen );
    opacity2.setCaption( "Velocity opacity: " + kvs::String::ToString( velocity.opacity ) );
    opacity2.setWidth( 150 );
    opacity2.setMargin( 10 );
    opacity2.setValue( velocity.opacity );
    opacity2.setRange( 0, 1 );
    opacity2.anchorToBottom( &opacity );
    opacity2.valueChanged(
        [&]() {
            velocity.opacity = opacity2.value();
            opacity2.setCaption( "Velocity opacity: " + kvs::String::ToString( velocity.opacity ) );
            auto* scene = screen.scene();
            auto* object1 = kvs::PolygonObject::DownCast( scene->object( "Velocity" ) );
            auto* object2 = new kvs::PolygonObject();           
            object2->shallowCopy( *object1 );
            object2->setName( "Velocity" );
            object2->setOpacity( int( velocity.opacity * 255 + 0.5 ) );
            scene->replaceObject( "Velocity", object2 );
        } );
    opacity2.show();

    kvs::Slider repetition( &screen );
    repetition.setCaption( "Repetition" );
    repetition.setWidth( 150 );
    repetition.setMargin( 10 );
    repetition.setValue( 20 );
    repetition.setRange( 1, 100 );
    repetition.anchorToBottom( &opacity2 );
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
      auto* renderer = local::SSAOStochasticPolygonRenderer::DownCast( scene->renderer( "MagneticRenderer" ) );
      auto* renderer2 = local::SSAOStochasticPolygonRenderer::DownCast( scene->renderer( "VelocityRenderer" ) );
      renderer->setEdgeFactor( edge );
      renderer2->setEdgeFactor( edge );
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

    kvs::OrientationAxis axis( &screen, screen.scene() );
    axis.setBoxType( kvs::OrientationAxis::SolidBox );
    axis.anchorToBottomRight();
    axis.show();
                
    return app.run();
}
