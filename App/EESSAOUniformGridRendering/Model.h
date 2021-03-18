#pragma once
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVectorToScalar>
#include <kvs/StochasticUniformGridRenderer>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticUniformGridRenderer.h>
#include "Input.h"


namespace local
{

class Model : public local::Input
{
public:
    using AORenderer = AmbientOcclusionRendering::SSAOStochasticUniformGridRenderer;
    using Renderer = kvs::StochasticUniformGridRenderer;

private:
    mutable kvs::StructuredVolumeObject* m_cached_volume = nullptr;

public:
    Model( local::Input& input ): local::Input( input ) {}

    const kvs::StructuredVolumeObject* cachedVolume() const
    {
        return m_cached_volume;
    }

    kvs::StructuredVolumeObject* import( const bool cache = true )
    {
        if ( m_cached_volume ) { delete m_cached_volume; }

        using Volume = kvs::StructuredVolumeObject;
        Volume* volume = new kvs::StructuredVolumeImporter( Input::filename );
        if ( volume->veclen() != 1 )
        {
            Volume* scalar = new kvs::StructuredVectorToScalar( volume );
            delete volume;
            volume = scalar;
        }

        if ( cache ) { m_cached_volume = volume; }

        return volume;
    }

    kvs::RendererBase* renderer()
    {
        if ( Input::ao )
        {
            auto* renderer = new AORenderer();
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
            auto* renderer = new Renderer();
            renderer->setName( "Renderer" );
            renderer->setTransferFunction( Input::tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
        }
    }
};

} // end of namespace local
