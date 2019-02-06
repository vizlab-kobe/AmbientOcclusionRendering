/*****************************************************************************/
/**
 *  @file   main.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include <kvs/Message>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/LineObject>
#include <kvs/Streamline>
#include <kvs/TornadoVolumeData>
#include <kvs/ShaderSource>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/KeyPressEventListener>
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticStylizedLineRenderer.h>

typedef AmbientOcclusionRendering::SSAOStochasticStylizedLineRenderer Renderer;

class Slider : public kvs::Slider
{
public:
    Slider( kvs::glut::Screen* screen ) : kvs::Slider( screen ) {}
    void valueChanged()
    {
        kvs::glut::Screen* s = static_cast<kvs::glut::Screen*>( screen() );
        Renderer* renderer = Renderer::DownCast( s->scene()->renderer() );
        renderer->setOpacity( kvs::Math::Round( value() * 255 ) );
    }
};

class KeyEvent : public kvs::KeyPressEventListener
{
    Slider& m_slider;

public:
    KeyEvent( Slider& slider ): m_slider( slider ) {}

    void update( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        case kvs::Key::i:
            if ( m_slider.isShown() ) { m_slider.hide(); }
            else { m_slider.show(); }
            break;
        default:
            break;
        }
    }
};

int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");

    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setGeometry( 0, 0, 512, 512 );
    screen.setTitle( "SSAOStochasticStylizedLineRenderer" );
    screen.show();

    kvs::StructuredVolumeObject* volume = new kvs::TornadoVolumeData( kvs::Vec3u::All( 32 ) );

    std::vector<kvs::Real32> v;
    kvs::Vector3i min_coord( 15, 15,  0 );
    kvs::Vector3i max_coord( 20, 20, 30 );
    for ( int k = min_coord.z(); k < max_coord.z(); k++ )
    {
        for ( int j = min_coord.y(); j < max_coord.y(); j++ )
        {
            for ( int i = min_coord.x(); i < max_coord.x(); i++ )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::PointObject* point = new kvs::PointObject;
    point->setCoords( kvs::ValueArray<kvs::Real32>( v ) );

    const kvs::TransferFunction transfer_function( 256 );
    kvs::LineObject* object = new kvs::Streamline( volume, point, transfer_function );

    delete volume;
    delete point;

    Renderer* renderer = new Renderer();
    renderer->setNumberOfSamplingPoints( 256 );
    renderer->setSamplingSphereRadius( 0.5 );
    renderer->setShader( kvs::Shader::BlinnPhong() );
    renderer->setOpacity( 128 );
    renderer->setRepetitionLevel( 50 );
    renderer->setEnabledLODControl( true );
    renderer->enableShading();

    screen.registerObject( object, renderer );

    Slider slider( &screen );
    slider.setCaption( "Opacity" );
    slider.setX( 0 );
    slider.setY( 0 );
    slider.setRange( 0, 1 );
    slider.setValue( 0.5 );
    slider.show();

    kvs::ScreenCaptureEvent capture_event;
    screen.addEvent( &capture_event );

    KeyEvent key_event( slider );
    screen.addEvent( &key_event );

    return app.run();
}
