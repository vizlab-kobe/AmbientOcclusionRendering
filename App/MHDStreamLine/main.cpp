#include <kvs/Message>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/LineObject>
#include <kvs/LineRenderer>
#include <kvs/StochasticLineRenderer>
#include <kvs/StylizedLineRenderer>
#include <kvs/Streamline>
#include <kvs/TornadoVolumeData>
#include <kvs/DivergingColorMap>
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/glut/TransferFunctionEditor>
#include <kvs/ShaderSource>
#include <StochasticStreamline/Lib/Streamline.h>
#include <kvs/glut/OrientationAxis>
#include <kvs/RGBFormulae>
#include <kvs/PaintEventListener>
#include <kvs/ScreenCaptureEvent>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>

void Scale( kvs::StructuredVolumeObject* volume, float scale )
{
  kvs::ValueArray<float> values = volume->values().asValueArray<float>();
  size_t nvalues = values.size();
  for ( size_t i = 0; i < nvalues; i++ )
    {
      values[i] *= scale;
    }

  volume->setValues( values );
  volume->updateMinMaxValues();
}

namespace
{

const kvs::Vec3ui VolumeResolution = kvs::Vec3ui( 32, 32, 32 );
const kvs::Vec3i SeedCoordMin = kvs::Vec3i( 0, 0,  0 );
const kvs::Vec3i SeedCoordMax = kvs::Vec3i( 250, 250, 250 );

const size_t Opacity = 128; // [0,255]
const size_t RepeatLevel = 40;

kvs::Xform X(
    kvs::Xform::Translation(
        kvs::Vec3( 0, 0.2, 0 ) ) *
    kvs::Xform::Rotation(
        kvs::Mat3::RotationX( -45 ) *
        kvs::Mat3::RotationY( 10 ) *
        kvs::Mat3::RotationZ( 50 ) ) );

kvs::StructuredVolumeObject* GenerateTornadoVolumeData()
{
    return new kvs::TornadoVolumeData( VolumeResolution );
}

kvs::PointObject* GenerateSeedPoints()
{
    std::vector<kvs::Real32> v;
    for ( int k = SeedCoordMin.z(); k < SeedCoordMax.z(); k+=30 )
    {
        for ( int j = SeedCoordMin.y(); j < SeedCoordMax.y(); j+=30 )
        {
            for ( int i = SeedCoordMin.x(); i < SeedCoordMax.x(); i+=30 )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::PointObject* point = new kvs::PointObject;
    point->setCoords( kvs::ValueArray<kvs::Real32>( v ) );
    return point;
}

}

namespace
{

void Line(
    kvs::glut::Screen& screen,
    const kvs::StructuredVolumeObject* volume,
    const kvs::PointObject* point,
    const kvs::TransferFunction& tfunc )
{
    const float line_width = 3.0f;

    kvs::LineObject* object = new kvs::Streamline( volume, point, tfunc );
    object->setSize( line_width );
    object->multiplyXform( X );

    kvs::StochasticLineRenderer* renderer = new kvs::StochasticLineRenderer();
    renderer->setShader( kvs::Shader::BlinnPhong() );
    renderer->setOpacity( ::Opacity );
    renderer->setRepetitionLevel( ::RepeatLevel );
    renderer->setEnabledLODControl( true );
    renderer->enableShading();

    screen.registerObject( object, renderer );
}

void Tube(
    kvs::glut::Screen& screen,
    const kvs::StructuredVolumeObject* volume,
    const kvs::PointObject* point,
    const kvs::TransferFunction& tfunc )
{
    const float line_width = 3.0f;

    kvs::LineObject* object = new kvs::Streamline( volume, point, tfunc );
    object->setSize( line_width );
    object->multiplyXform( X );

    typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;
    Renderer* renderer = new Renderer();
    renderer->setShader( kvs::Shader::BlinnPhong() );
    //renderer->setOpacity( ::Opacity );
    renderer->setRepetitionLevel( ::RepeatLevel );
    renderer->setEnabledLODControl( true );
    renderer->enableShading();

    screen.registerObject( object, renderer );
}

void TubeNew(
    kvs::glut::Screen& screen,
    const kvs::StructuredVolumeObject* volume,
    const kvs::PointObject* point,
    const kvs::TransferFunction& tfunc )
{
    typedef StochasticStreamline::Streamline Mapper;
    typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;

    const float line_width = 3.0f;

    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( point );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    //else if ( integration_direction_num == -1 ) mapper->setIntegrationDirection( Mapper::BackwardDirection );
    mapper->setTransferFunction( tfunc );

    kvs::LineObject* object = mapper->exec( volume );
    //kvs::LineObject* object = new Mapper( volume, point, tfunc );
    //object->multiplyXform( X );

    Renderer* renderer = new Renderer();
    renderer->setName("Renderer");
    renderer->setTransferFunction( tfunc );
    renderer->setShader( kvs::Shader::BlinnPhong() );
    renderer->setRepetitionLevel( ::RepeatLevel );
    renderer->setEnabledLODControl( true );
    renderer->enableShading();

    screen.registerObject( object, renderer );
}

}

class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{
public:

    TransferFunctionEditor( kvs::glut::Screen* screen ):
        kvs::glut::TransferFunctionEditor( screen ){}

    void apply()
    {
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        Renderer* renderer = Renderer::DownCast( scene->renderer( "Renderer" ) );
        renderer->setTransferFunction( transferFunction() );
        screen()->redraw();
    }
};


class PaintEvent : public kvs::PaintEventListener
{
    void update()
    {
        static size_t counter = 1;
        static float time = 0.0f;

        time += scene()->renderer("Renderer")->timer().msec();
        if ( counter++ == 10 )
        {
            std::cout << "Rendering time: " << time / counter << " [msec]" << std::endl;
            counter = 1;
            time = 0.0f;
        }
    }
};


int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::glut::Application app( argc, argv );

    //kvs::StructuredVolumeObject* volume = GenerateTornadoVolumeData();
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( argv[1]);
    Scale(volume,100);

    kvs::PointObject* point = GenerateSeedPoints();
    kvs::TransferFunction tfunc( kvs::DivergingColorMap::CoolWarm( 256 ) );
    // kvs::TransferFunction tfunc( kvs::RGBFormulae::PM3D( 256 ) );

/*
    kvs::glut::Screen screen1( &app );
    screen1.setTitle( "Stochastic line" );
    screen1.setPosition( 0, 0 );
    ::Line( screen1, volume, point, tfunc );

    kvs::glut::Screen screen2( &app );
    screen2.setTitle( "Stochastic Tubeline" );
    screen2.setPosition( screen1.x() + screen1.width(), 0 );
    ::Tube( screen2, volume, point, tfunc );
*/
    kvs::glut::Screen screen3( &app );
    screen3.setTitle( "Stochastic Tubeline with Transfer function" );
    screen3.setBackgroundColor( kvs::RGBColor::White() );
//    screen3.setPosition( screen2.x() + screen2.width(), 0 );
    ::TubeNew( screen3, volume, point, tfunc );

//    delete volume;
    delete point;

//    screen1.show();
//    screen2.show();
    screen3.show();


    kvs::glut::OrientationAxis orientation_axis(&screen3);
    orientation_axis.setBoxType(kvs::glut::OrientationAxis::SolidBox);
    orientation_axis.show();
     
    TransferFunctionEditor editor( &screen3 );
    editor.setTransferFunction( tfunc );
    editor.setVolumeObject( volume );
    editor.show();

    PaintEvent paint_event;
    kvs::ScreenCaptureEvent capture_event;
    screen3.addEvent( &paint_event );
    screen3.addEvent( &capture_event );

    return app.run();
}
