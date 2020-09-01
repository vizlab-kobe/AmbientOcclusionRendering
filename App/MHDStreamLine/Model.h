#pragma once
#include "Input.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/LineObject>
#include <kvs/PointObject>
#include <kvs/RendererBase>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>


namespace local
{

class Model : public Input
{
private:
    mutable kvs::StructuredVolumeObject* m_cached_volume;

public:
    typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
    typedef StochasticStreamline::StochasticTubeRenderer NoSSAORenderer;

public:
    Model( local::Input& input ): Input( input ), m_cached_volume( NULL ) {}

    const kvs::StructuredVolumeObject* cachedVolume() const { return m_cached_volume; }
    kvs::StructuredVolumeObject* import( const bool cache = true ) const;
    kvs::LineObject* streamline( const kvs::StructuredVolumeObject* volume = NULL ) const;
    kvs::RendererBase* renderer() const;

private:
    kvs::PointObject* generate_seed_points() const;
};

} // end of namespace local
