#pragma once
#include <kvs/ObjectBase>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/StochasticRendererBase>
#include <kvs/StochasticRenderingEngine>
#include <kvs/Deprecated>
#include "AmbientOcclusionBuffer.h"
#include "SSAOStochasticRenderingCompositor.h"


namespace AmbientOcclusionRendering
{

class SSAOStochasticRendererBase : public kvs::StochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticRendererBase, Renderer );
    friend class SSAOStochasticRenderingCompositor;

private:
    using BaseClass = kvs::StochasticRendererBase;
    AmbientOcclusionBuffer m_ao_buffer; /// ambient occlusion buffer

public:
    SSAOStochasticRendererBase( kvs::StochasticRenderingEngine* engine ):
        kvs::StochasticRendererBase( engine ) {}

    virtual void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    const AmbientOcclusionBuffer& aoBuffer() const { return m_ao_buffer; }
    AmbientOcclusionBuffer& aoBuffer() { return m_ao_buffer; }

    void setKernelRadius( const float radius ) { m_ao_buffer.setKernelRadius( radius ); }
    void setKernelSize( const size_t nsamples ) { m_ao_buffer.setKernelSize( nsamples ); }
    void setDrawingOcclusionFactorEnabled( const bool enabled = true ) { m_ao_buffer.setDrawingOcclusionFactorEnabled( enabled ); }
    kvs::Real32 kernelRadius() const { return m_ao_buffer.kernelRadius(); }
    size_t kernelSize() const { return m_ao_buffer.kernelSize(); }

    KVS_DEPRECATED( void setSamplingSphereRadius( const float radius ) ) { this->setKernelRadius( radius ); }
    KVS_DEPRECATED( void setNumberOfSamplingPoints( const size_t nsamples ) ) { this->setKernelSize( nsamples ); }
    KVS_DEPRECATED( kvs::Real32 samplingSphereRadius() const ) { return this->kernelRadius(); }
    KVS_DEPRECATED( size_t numberOfSamplingPoints() const ) { return this->kernelSize(); }
};

} // end of namespace AmbientOcclusionRendering
