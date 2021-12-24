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

#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/StochasticPolygonRenderer>
#include <kvs/StochasticRenderingCompositor>

#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticRenderingCompositor.h>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>


struct Model
{
    using Importer = kvs::StructuredVolumeImporter;
    using VolumeObject = kvs::StructuredVolumeObject;
    using PolygonObject = kvs::PolygonObject;
    using LineObject = kvs::LineObject;
    using PolygonRenderer = kvs::StochasticPolygonRenderer;
    using LineRenderer = StochasticStreamline::StochasticTubeRenderer;
    using Compositor = kvs::StochasticRenderingCompositor;
    using SSAOPolygonRenderer = AmbientOcclusionRendering::SSAOStochasticPolygonRenderer;
    using SSAOLineRenderer = AmbientOcclusionRendering::SSAOStochasticTubeRenderer;
    using SSAOCompositor = AmbientOcclusionRendering::SSAOStochasticRenderingCompositor;

    bool ssao = true; ///< SSAO flag
    bool occlusion = false; ///< occlusion factor drawing flag
    float radius = 3.0f; ///< radius of point sampling region for SSAM
    size_t points = 256; ///< number of points used for SSAO
    float intensity = 2.0f; ///< SSAO intensity

    bool lod = true;
    size_t repeats = 10;
    float opacity = 0.5f;

    VolumeObject* scalar_volume = nullptr;
    VolumeObject* vector_volume = nullptr;
    Compositor* compositor = nullptr;

    VolumeObject* import(
        const std::string filename,
        const bool scalarization = false,
        const float scaling = 1.0f )
    {
        std::unique_ptr<VolumeObject> volume( new Importer( filename ) );
        if ( scalarization )
        {
            return this->scalarize( volume.get() );
        }
        else
        {
            return this->scale( volume.get(), scaling );
        }
    }

    PolygonObject* isosurface( const VolumeObject* volume )
    {
        double min_value = volume->minValue();
        double max_value = volume->maxValue();
        double isovalue = ( max_value + min_value ) * 0.1;  // Isolevel parameter.
        const auto n = kvs::PolygonObject::VertexNormal;
        const auto d = false;
        const auto t = kvs::TransferFunction( 256 );
        auto* polygon = new kvs::Isosurface( volume, isovalue, n, d, t );
        polygon->setName( "Polygon" );
        polygon->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );
        return polygon;
    }

    LineObject* streamline( const VolumeObject* volume )
    {
        auto seed_points = [&] ( kvs::Vec3i stride )
        {
            const auto min_coord = volume->minObjectCoord();
            const auto max_coord = volume->maxObjectCoord();

            std::vector<kvs::Real32> v;
            for ( int k = min_coord.z(); k <= max_coord.z(); k += stride.z() )
            {
                for ( int j = min_coord.y(); j <= max_coord.y(); j += stride.y() )
                {
                    for ( int i = min_coord.x(); i <= max_coord.x(); i += stride.x() )
                    {
                        v.push_back( static_cast<kvs::Real32>(i) );
                        v.push_back( static_cast<kvs::Real32>(j) );
                        v.push_back( static_cast<kvs::Real32>(k) );
                    }
                }
            }

            auto* seeds = new kvs::PointObject;
            seeds->setCoords( kvs::ValueArray<kvs::Real32>( v ) );
            return seeds;
        };

        using Mapper = local::Streamline;
        Mapper* mapper = new Mapper();
        mapper->setSeedPoints( seed_points( { 30, 30, 30 } ) );
        mapper->setIntegrationInterval( 0.1 );
        mapper->setIntegrationMethod( Mapper::RungeKutta4th );
        mapper->setIntegrationDirection( Mapper::ForwardDirection );
        mapper->setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
        return mapper->exec( volume );
    }

    kvs::RendererBase* polygonRenderer()
    {
        if ( ssao )
        {
            auto* renderer = new SSAOPolygonRenderer();
            renderer->setName( "PolygonRenderer" );
            return renderer;
        }
        else
        {
            auto* renderer = new PolygonRenderer();
            renderer->setName( "PolygonRenderer" );
            return renderer;
        }
    }

    kvs::RendererBase* lineRenderer()
    {
        if ( ssao )
        {
            auto* renderer = new SSAOLineRenderer();
            renderer->setName( "LineRenderer" );
            return renderer;
        }
        else
        {
            auto* renderer = new LineRenderer();
            renderer->setName( "LineRenderer" );
            return renderer;
        }
    }

    Compositor* createCompositor( kvs::Scene* scene )
    {
        if ( ssao )
        {
            using SSAOCompositor = AmbientOcclusionRendering::SSAOStochasticRenderingCompositor;
            auto* c = new SSAOCompositor( scene );
            c->enableLODControl();
            c->setRepetitionLevel( repeats );
            c->aoBuffer().setKernelRadius( radius );
            c->aoBuffer().setKernelSize( points );
            c->aoBuffer().setIntensity( intensity );
            c->aoBuffer().setDrawingOcclusionFactorEnabled( occlusion );
            return c;
        }
        else
        {
            auto* c = new Compositor( scene );
            c->enableLODControl();
            c->setRepetitionLevel( repeats );
            return c;
        }
    }

private:
    VolumeObject* scalarize( const VolumeObject* volume )
    {
        auto* scalar_volume = new kvs::StructuredVectorToScalar( volume );
        return scalar_volume;
    }

    VolumeObject* scale( const VolumeObject* volume, const float scale )
    {
        auto* scaled_volume = new kvs::StructuredVolumeObject();
        scaled_volume->shallowCopy( *volume );

        auto values = volume->values().asValueArray<float>();
        for ( auto& value : values ) { value *= scale; }
        scaled_volume->setValues( values );
        scaled_volume->updateMinMaxValues();

        return scaled_volume;
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
    kvs::ShaderSource::AddSearchPath( "../../../StochasticStreamline/Lib" );

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle( "SSAOStochasticRenderingCompositor Streamline and Polygon" );
    screen.show();

    // Parameters.
    Model model;

    // Import volume object.
    const std::string magnetic_file = argv[1];
    const std::string velocity_file = argv[2];
    model.scalar_volume = model.import( magnetic_file, true );
    model.vector_volume = model.import( velocity_file, false, 100.0f );

    // Register objects and renderers
    screen.registerObject( model.isosurface( model.scalar_volume ), model.polygonRenderer() );
    screen.registerObject( model.streamline( model.vector_volume ), model.lineRenderer() );

    // Create stochastic rendering compositor.
    model.compositor = model.createCompositor( screen.scene() );
    screen.setEvent( model.compositor );

    // Widgets.
    kvs::CheckBox ssao_check_box( &screen );
    ssao_check_box.setCaption( "SSAO" );
    ssao_check_box.setState( model.ssao );
    ssao_check_box.setMargin( 10 );
    ssao_check_box.show();
    ssao_check_box.stateChanged( [&] ()
    {
        model.ssao = ssao_check_box.state();
        screen.scene()->replaceRenderer( "PolygonRenderer", model.polygonRenderer() );
        screen.scene()->replaceRenderer( "LineRenderer", model.lineRenderer() );

        if ( model.compositor ) { delete model.compositor; }
        model.compositor = model.createCompositor( screen.scene() );
        screen.setEvent( model.compositor );
        screen.addEvent( &ssao_check_box );
    } );

    return app.run();
}
