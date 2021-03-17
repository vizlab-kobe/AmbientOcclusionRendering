#pragma once
#include "Input.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/LineObject>
#include <kvs/PointObject>
#include <kvs/RendererBase>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>
#include "Streamline.h"


namespace local
{

class Model : public local::Input
{

public:
    using AORenderer = AmbientOcclusionRendering::SSAOStochasticTubeRenderer;
    using Renderer = StochasticStreamline::StochasticTubeRenderer;

private:
    mutable kvs::StructuredVolumeObject* m_cached_volume = nullptr;

public:
    Model( local::Input& input ): local::Input( input ) {}

    const kvs::StructuredVolumeObject* cachedVolume() const
    {
        return m_cached_volume;
    }

    kvs::StructuredVolumeObject* import( const bool cache = true ) const
    {
        if ( m_cached_volume ) { delete m_cached_volume; }

        auto* volume = new kvs::StructuredVolumeImporter( Input::filename );
        if ( volume->veclen() != 3 ) { return nullptr; }

        auto values = volume->values().asValueArray<float>();
        for ( auto& value : values ) { value *= Input::scale; }
        volume->setValues( values );
        volume->updateMinMaxValues();

        if ( cache ) { m_cached_volume = volume; }

        return volume;
    }

    kvs::LineObject* streamline( const bool cache = true ) const
    {
        using Mapper = local::Streamline;
        const auto* volume = ( !m_cached_volume ) ? this->import( cache ) : m_cached_volume;

        kvs::SharedPointer<kvs::PointObject> seeds( this->generate_seed_points() );
        Mapper* mapper = new Mapper();
        mapper->setSeedPoints( seeds.get() );
        mapper->setIntegrationInterval( 0.1 );
        mapper->setIntegrationMethod( Mapper::RungeKutta4th );
        mapper->setIntegrationDirection( Mapper::ForwardDirection );
        mapper->setTransferFunction( Input::tfunc );
        return mapper->exec( volume );
    }

    kvs::RendererBase* renderer() const
    {
        if ( local::Input::ao )
        {
            AORenderer* renderer = new AORenderer();
            renderer->setName( "Renderer" );
            renderer->setTransferFunction( Input::tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
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
            Renderer* renderer = new Renderer();
            renderer->setName( "Renderer" );
            renderer->setTransferFunction( Input::tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
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
};

} // end of namespace local
