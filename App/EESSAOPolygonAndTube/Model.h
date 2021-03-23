#pragma once
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/LineObject>
#include <kvs/PointObject>
#include <kvs/RendererBase>
#include <kvs/StochasticPolygonRenderer>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include "Input.h"
#include "Streamline.h"
#include "SSAOStochasticPolygonRenderer.h"
#include "SSAOStochasticTubeRenderer.h"


namespace local
{

class Model : public local::Input
{
public:
    using AOIsosurfaceRenderer = local::SSAOStochasticPolygonRenderer;
    using IsosurfaceRenderer = kvs::StochasticPolygonRenderer;

    using AOStreamlineRenderer = local::SSAOStochasticTubeRenderer;
    using StreamlineRenderer = StochasticStreamline::StochasticTubeRenderer;

private:
    mutable kvs::StructuredVolumeObject* m_cached_volumes[2] = { nullptr, nullptr };

public:
    Model( local::Input& input ): local::Input( input ) {}
    ~Model()
    {
        if ( m_cached_volumes[0] ) { delete m_cached_volumes[0]; }
        if ( m_cached_volumes[1] ) { delete m_cached_volumes[1]; }
    }

    const kvs::StructuredVolumeObject* cachedVolume( const MappingMethod method ) const
    {
        return m_cached_volumes[ method ];
    }

    kvs::StructuredVolumeObject* import( const MappingMethod method, const bool cache = true ) const
    {
        switch ( method )
        {
        case MappingMethod::Isosurface: return this->import_isosurface( cache );
        case MappingMethod::Streamline: return this->import_streamline( cache );
        default: return nullptr;
        }
    }

    kvs::PolygonObject* isosurface( const float isovalue, const bool cache = true ) const
    {
        const MappingMethod method = MappingMethod::Isosurface;
        const auto* volume = ( !m_cached_volumes[ method ] ) ? this->import( method, cache ) : m_cached_volumes[ method ];

        const auto n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
        const auto t = Input::tfunc;
        auto* polygon = new kvs::Isosurface( volume, isovalue, n, d, t );
        polygon->setName( "IsosurfaceObject" );
        polygon->setOpacity( kvs::Math::Clamp( int( Input::opacity * 255.0 ), 0, 255 ) );
        return polygon;
    }

    kvs::LineObject* streamline( const bool cache = true ) const
    {
        using Mapper = local::Streamline;
        const MappingMethod method = MappingMethod::Streamline;
        const auto* volume = ( !m_cached_volumes[ method ] ) ? this->import( method, cache ) : m_cached_volumes[ method ];

        kvs::SharedPointer<kvs::PointObject> seeds( this->generate_seed_points() );
        Mapper* mapper = new Mapper();
        mapper->setName( "StreamlineObject" );
        mapper->setSeedPoints( seeds.get() );
        mapper->setIntegrationInterval( 0.1 );
        mapper->setIntegrationMethod( Mapper::RungeKutta4th );
        mapper->setIntegrationDirection( Mapper::ForwardDirection );
        mapper->setTransferFunction( Input::tfunc );
        return mapper->exec( volume );
    }

    kvs::RendererBase* renderer( const MappingMethod method ) const
    {
        switch ( method )
        {
        case MappingMethod::Isosurface: return this->isosurface_renderer();
        case MappingMethod::Streamline: return this->streamline_renderer();
        default: return nullptr;
        }
    }

private:
    kvs::PointObject* generate_seed_points() const
    {
        std::vector<kvs::Real32> v;
        for ( int k = Input::min_coord.z(); k < Input::max_coord.z(); k += Input::stride.z() )
        {
            for ( int j = Input::min_coord.y(); j < Input::max_coord.y(); j += Input::stride.y() )
            {
                for ( int i = Input::min_coord.x(); i < Input::max_coord.x(); i += Input::stride.x() )
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
    }

    kvs::StructuredVolumeObject* import_isosurface( const bool cache = true ) const
    {
        const MappingMethod method = Input::MappingMethod::Isosurface;
        auto* cached_volume = m_cached_volumes[ method ];
        if ( cached_volume ) { delete cached_volume; }

        using Volume = kvs::StructuredVolumeObject;
        Volume* volume = new kvs::StructuredVolumeImporter( Input::filenames[ method ] );
        if ( volume->veclen() != 1 )
        {
            Volume* scalar = new kvs::StructuredVectorToScalar( volume );
            delete volume;
            volume = scalar;
        }
        volume->updateMinMaxValues();

        if ( cache ) { cached_volume = volume; }

        return volume;
    }

    kvs::StructuredVolumeObject* import_streamline( const bool cache = true ) const
    {
        const MappingMethod method = Input::MappingMethod::Streamline;
        auto* cached_volume = m_cached_volumes[ method ];
        if ( cached_volume ) { delete cached_volume; }

        auto* volume = new kvs::StructuredVolumeImporter( Input::filenames[ method ] );
        if ( volume->veclen() != 3 ) { return nullptr; }

        auto values = volume->values().asValueArray<float>();
        for ( auto& value : values ) { value *= Input::scale; }
        volume->setValues( values );
        volume->updateMinMaxValues();

        if ( cache ) { cached_volume = volume; }

        return volume;
    }

    kvs::RendererBase* isosurface_renderer() const
    {
        using AORenderer = AOIsosurfaceRenderer;
        using Renderer = IsosurfaceRenderer;

        if ( Input::ao )
        {
            auto* renderer = new AORenderer();
            renderer->setName( "IsosurfaceRenderer" );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setSamplingSphereRadius( Input::radius );
            renderer->setNumberOfSamplingPoints( Input::points );
            renderer->setEdgeFactor( Input::edge );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "IsosurfaceRenderer" );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
        }
    }

    kvs::RendererBase* streamline_renderer() const
    {
        using AORenderer = AOStreamlineRenderer;
        using Renderer = StreamlineRenderer;

        if ( local::Input::ao )
        {
            auto* renderer = new AORenderer();
            renderer->setName( "StreamlineRenderer" );
            renderer->setTransferFunction( Input::tfunc );
//            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->setSamplingSphereRadius( Input::radius );
            renderer->setNumberOfSamplingPoints( Input::points );
            renderer->setEdgeFactor( Input::edge );
            renderer->enableShading();
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "StreamlineRenderer" );
            renderer->setTransferFunction( Input::tfunc );
//            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
        }
    }
};

} // end of namespace local
