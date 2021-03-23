#pragma once
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/PointObject>
#include <kvs/RendererBase>
#include <kvs/StochasticPolygonRenderer>
#include <kvs/String>
#include "Input.h"
#include "SSAOStochasticPolygonRenderer.h"


namespace local
{

class Model : public local::Input
{
public:
    using AORenderer = local::SSAOStochasticPolygonRenderer;
    using Renderer = kvs::StochasticPolygonRenderer;

private:
    mutable kvs::StructuredVolumeObject* m_cached_volumes[2] = { nullptr, nullptr };

public:
    Model( local::Input& input ): local::Input( input ) {}
    ~Model()
    {
        if ( m_cached_volumes[0] ) { delete m_cached_volumes[0]; }
        if ( m_cached_volumes[1] ) { delete m_cached_volumes[1]; }
    }

    const kvs::StructuredVolumeObject* cachedVolume( const size_t index ) const
    {
        return m_cached_volumes[ index ];
    }

    kvs::StructuredVolumeObject* import( const size_t index, const bool cache = true ) const
    {
        auto* cached_volume = m_cached_volumes[ index ];
        if ( cached_volume ) { delete cached_volume; }

        using Volume = kvs::StructuredVolumeObject;
        Volume* volume = new kvs::StructuredVolumeImporter( Input::filenames[ index ] );
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

    kvs::PolygonObject* isosurface( const size_t index, const float isovalue, const bool cache = true ) const
    {
        const auto* volume = ( !m_cached_volumes[ index ] ) ? this->import( index, cache ) : m_cached_volumes[ index ];

        const auto n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
        const auto t = Input::tfuncs[ index ];
        auto* polygon = new kvs::Isosurface( volume, isovalue, n, d, t );
        polygon->setName( "Object" + kvs::String::From( index ) );
        polygon->setOpacity( kvs::Math::Clamp( int( Input::opacities[index] * 255.0 ), 0, 255 ) );
        return polygon;
    }

    kvs::RendererBase* renderer( const size_t index ) const
    {
        if ( Input::ao )
        {
            auto* renderer = new AORenderer();
            renderer->setName( "Renderer" + kvs::String::From( index ) );
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
            renderer->setName( "Renderer" + kvs::String::From( index ) );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
        }
    }
};

} // end of namespace local
