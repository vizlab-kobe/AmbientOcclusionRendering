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
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/glut/TransferFunctionEditor>
#include <StochasticStreamline/Lib/Streamline.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>

typedef StochasticStreamline::Streamline Streamline;
typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;

class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{
public:

    TransferFunctionEditor( kvs::glut::Screen* screen ):
        kvs::glut::TransferFunctionEditor( screen ){}

    void apply()
    {
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        Renderer* renderer = Renderer::DownCast( scene->renderer( "Renderer" ) );
        renderer->setTransferFunction( transferFunction() );
        screen()->redraw();
    }
};

int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");

    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setGeometry( 0, 0, 512, 512 );
    screen.setTitle( "SSAOStochasticTubeRenderer" );
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
    kvs::LineObject* object = new Streamline( volume, point, transfer_function );
    delete point;

    Renderer* renderer = new Renderer();
    renderer->setName( "Renderer" );
    renderer->setTransferFunction( transfer_function );
    renderer->setShader( kvs::Shader::BlinnPhong() );
    renderer->setRepetitionLevel( 50 );
    renderer->setEnabledLODControl( true );
    renderer->enableShading();

    screen.registerObject( object, renderer );

    TransferFunctionEditor editor( &screen );
    editor.setTransferFunction( transfer_function );
    editor.setVolumeObject( volume );
    editor.show();

    kvs::ScreenCaptureEvent event;
    screen.addEvent( &event );

    return app.run();
}
