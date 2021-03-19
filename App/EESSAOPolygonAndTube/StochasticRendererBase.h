#pragma once
#include <kvs/ObjectBase>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/StochasticRendererBase>
#include <kvs/StochasticRenderingEngine>
#include "AmbientOcclusionBuffer.h"
#include "StochasticRenderingCompositor.h"


namespace local
{

class StochasticRendererBase : public kvs::StochasticRendererBase
{
    kvsModule( local::StochasticRendererBase, Renderer );
    friend class local::StochasticRenderingCompositor;

private:
    using BaseClass = kvs::StochasticRendererBase;
    local::AmbientOcclusionBuffer m_ao_buffer; /// ambient occlusion buffer

public:
    StochasticRendererBase( kvs::StochasticRenderingEngine* engine ):
        kvs::StochasticRendererBase( engine ) {}

    virtual void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }
    const kvs::StochasticRenderingEngine& engine() const { return BaseClass::engine(); }

protected:
    kvs::StochasticRenderingEngine& engine() { return BaseClass::engine(); }
};

} // end of namespace local
