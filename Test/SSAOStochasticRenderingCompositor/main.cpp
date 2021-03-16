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

#include <kvs/StochasticPolygonRenderer>
//#include <kvs/StochasticRenderingCompositor>
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>

#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>


kvs::Vec3i min_coord( 0, 0, 0 );
kvs::Vec3i max_coord( 250, 250, 250 );

kvs::PolygonObject* createIsosurface( std::string filename )
{
    auto* volume = new kvs::StructuredVolumeImporter( filename );
    auto* scalar_volume = new kvs::StructuredVectorToScalar( volume );

    double min_value = scalar_volume->minValue();
    double max_value = scalar_volume->maxValue();
    double isovalue = ( max_value + min_value ) * 0.1;  // Isolevel parameter.
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

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle("SSAOStochasticRenderingCompositor Streamline and Polygon");
    //screen.setSize( 1024, 1024 );
    screen.show();

    // Import volume object.
    std::string magnetic_file = argv[1];
    std::string velocity_file = argv[2];

//    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( velocity_file );
    auto* polygon = createIsosurface( magnetic_file );
    auto* streamline = createStreamline( velocity_file );

    // Declare SSAOStochasticPolygonRenderer
    auto* polygon_renderer = new AmbientOcclusionRendering::SSAOStochasticPolygonRenderer();
    polygon_renderer->setName( "PolygonRenderer" );
    polygon_renderer->setRepetitionLevel( 10 );

    // Declare SSAOStochasticTubeRenderer
    auto* tube_renderer = new AmbientOcclusionRendering::SSAOStochasticTubeRenderer();
    tube_renderer->setName( "TubeRenderer" );
    tube_renderer->setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    tube_renderer->setRepetitionLevel( 10 );

    // Register objects and renderers
    screen.registerObject( polygon, polygon_renderer );
//    screen.registerObject( streamline, tube_renderer );

    return app.run();
}
