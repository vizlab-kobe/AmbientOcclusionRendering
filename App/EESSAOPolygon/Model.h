#pragma once
#include <kvs/StochasticPolygonRenderer>
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/PolygonObject>
#include <kvs/TransferFunction>
#include <kvs/ColorMap>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>
#include "Input.h"


namespace local
{

/*===========================================================================*/
/**
 *  @brief  Model class manages SSAO parameters
 */
/*===========================================================================*/
class Model : public local::Input
{
public:
    using AORenderer = AmbientOcclusionRendering::SSAOStochasticPolygonRenderer;
    using Renderer = kvs::StochasticPolygonRenderer;

private:
    mutable kvs::StructuredVolumeObject* m_cached_volume = nullptr;

public:
    Model( local::Input& input ): local::Input( input ) {}
    ~Model() { if ( m_cached_volume ) { delete m_cached_volume; } }

    const kvs::StructuredVolumeObject* cachedVolume() const
    {
        return m_cached_volume;
    }

    kvs::StructuredVolumeObject* import( const bool cache = true ) const
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

    kvs::PolygonObject* isosurface( const float isovalue, const bool cache = true ) const
    {
        const auto* volume = ( !m_cached_volume ) ? this->import( cache ) : m_cached_volume;

        const auto n = kvs::PolygonObject::VertexNormal;
        const bool d = false;
        const auto t = Input::tfunc;
        auto* polygon = new kvs::Isosurface( volume, isovalue, n, d, t );
        polygon->setName( "Object" );
        polygon->setOpacity( kvs::Math::Clamp( int( Input::opacity * 255.0 ), 0, 255 ) );
        return polygon;
    }

    kvs::RendererBase* renderer() const
    {
        if ( Input::ao )
        {
            auto* renderer = new AORenderer();
            renderer->setName( "Renderer" );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->setKernelRadius( Input::radius );
            renderer->setKernelSize( Input::points );
            renderer->setEdgeFactor( Input::edge );
            renderer->enableShading();
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "Renderer" );
            renderer->setRepetitionLevel( Input::repeats );
            renderer->setLODControlEnabled( Input::lod );
            renderer->enableShading();
            return renderer;
        }
    }
};

} // end of namespace local
