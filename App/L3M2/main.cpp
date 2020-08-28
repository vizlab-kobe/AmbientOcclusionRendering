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
#include <StochasticStreamline/Lib/StochasticStylizedLineRenderer.h>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include "Streamline.h"
#include <kvs/glut/OrientationAxis>
#include <kvs/PaintEventListener>
#include <kvs/ScreenCaptureEvent>
#include "ReadBinary.h"
#include <kvs/RayCastingRenderer>

//grid resolution
const size_t dimx = 250;
const size_t dimy = 250;
const size_t dimz = 250;

const size_t veclen = 3;

const kvs::Vec3ui GridStride = kvs::Vec3ui( 30, 30, 30 );
const kvs::Vec3i SeedCoordMin = kvs::Vec3i( 0, 0,  0 );
const kvs::Vec3i SeedCoordMax = kvs::Vec3i( 250, 250, 250 );

const size_t Opacity = 128; // [0,255]
const size_t RepeatLevel = 40;

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

kvs::StructuredVolumeObject* CreateStructuredVolumeObject()
{   
    kvs::ValueArray<float> values_x = local::ReadBinary( "/Users/fujitayasuyuki/Work/Data/MHDData/data_L3M1to3_ortho/data/eMar03b.005.rfield.bx.n001574586.t00190", 4, true );
    kvs::ValueArray<float> values_y = local::ReadBinary( "/Users/fujitayasuyuki/Work/Data/MHDData/data_L3M1to3_ortho/data/eMar03b.005.rfield.by.n001574586.t00190", 4, true );
    kvs::ValueArray<float> values_z = local::ReadBinary( "/Users/fujitayasuyuki/Work/Data/MHDData/data_L3M1to3_ortho/data/eMar03b.005.rfield.bz.n001574586.t00190", 4, true );

    kvs::ValueArray<float> values( values_x.size() * 3 );
    
    for( int i = 0; i < values_x.size(); i ++ )
    {
        values[i*3] = values_x[i];
        values[i*3+1] = values_y[i];
        values[i*3+2] = values_z[i];
    }

    int count = 0;
    
    for( int i = 0; i < values.size(); i++ )
    {
        if( values[i] == 0 )
        {
            count++;
        }
    }
    
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeObject();
    volume->setVeclen( veclen );
    volume->setResolution( kvs::Vector3ui( dimx, dimy, dimz ) );
    volume->setValues( values );
    volume->setGridTypeToUniform();
    volume->updateMinMaxValues();
    return volume;
}
    
kvs::Xform X(
    kvs::Xform::Translation(
        kvs::Vec3( 0, 0.2, 0 ) ) *
    kvs::Xform::Rotation(
        kvs::Mat3::RotationX( -45 ) *
        kvs::Mat3::RotationY( 10 ) *
        kvs::Mat3::RotationZ( 50 ) ) );


kvs::PointObject* GenerateSeedPoints()
{
    std::vector<kvs::Real32> v;
    for ( int k = SeedCoordMin.z(); k < SeedCoordMax.z(); k += GridStride.z() )
    {
        for ( int j = SeedCoordMin.y(); j < SeedCoordMax.y(); j += GridStride.y() )
        {
            for ( int i = SeedCoordMin.x(); i < SeedCoordMax.x(); i += GridStride.x() )
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

void TubeNew(
    kvs::glut::Screen& screen,
    const kvs::StructuredVolumeObject* volume,
    const kvs::PointObject* point,
    const kvs::TransferFunction& tfunc )
{
    typedef local::Streamline Mapper;
    typedef StochasticStreamline::StochasticTubeRenderer Renderer;
    //typedef kvs::glsl::RayCastingRenderer Renderer;
    
    //const float line_width = 3.0f;

    kvs::LineObject* object = new Mapper( volume, point, tfunc );
    /*kvs::LineObject* object = new kvs::LineObject();
    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( point );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    mapper->setTransferFunction( tfunc );
    object = mapper->exec( volume );*/
    object->multiplyXform( X );

    Renderer* renderer = new Renderer();
    renderer->setName("Renderer");
    renderer->setTransferFunction( tfunc );
    renderer->setShader( kvs::Shader::BlinnPhong() );
    renderer->setRepetitionLevel( ::RepeatLevel );
    renderer->setEnabledLODControl( true );
    renderer->enableShading();
    screen.registerObject( object, renderer );
}


class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{
public:

    TransferFunctionEditor( kvs::glut::Screen* screen ):
        kvs::glut::TransferFunctionEditor( screen ){}

    void apply()
    {
        typedef StochasticStreamline::StochasticTubeRenderer Renderer;
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
            counter = 1;
            time = 0.0f;
        }
    }
};


int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::ShaderSource::AddSearchPath( "../../../StochasticStreamline/Lib" );
    kvs::glut::Application app( argc, argv );

    kvs::StructuredVolumeObject* volume = CreateStructuredVolumeObject();
    //new kvs::StructuredVolumeImporter( std::string(argv[1]));
    Scale(volume, 100);

    kvs::PointObject* point = GenerateSeedPoints();
    kvs::TransferFunction tfunc( kvs::DivergingColorMap::CoolWarm( 256 ) );

    kvs::glut::Screen screen( &app );
    screen.setTitle( "Stochastic Tubeline with Transfer function" );
    screen.setBackgroundColor( kvs::RGBColor::White() );

    ::TubeNew( screen, volume, point, tfunc );

    delete point;
    screen.show();
     
    TransferFunctionEditor editor( &screen );
    editor.setTransferFunction( tfunc );
    editor.setVolumeObject( volume );
    editor.show();

    kvs::glut::OrientationAxis orientation_axis( &screen );
    orientation_axis.setBoxType( kvs::glut::OrientationAxis::WiredBox );
    orientation_axis.show();
    
    PaintEvent paint_event;
    kvs::ScreenCaptureEvent capture_event;
    screen.addEvent( &paint_event );
    screen.addEvent( &capture_event );
    return app.run();
}
