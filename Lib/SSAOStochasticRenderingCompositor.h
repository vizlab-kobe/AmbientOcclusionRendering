/*****************************************************************************/
/**
 *  @file   StochasticRenderingCompositor.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/Shader>
#include <kvs/StochasticRenderingCompositor>
#include "AmbientOcclusionBuffer.h"


namespace AmbientOcclusionRendering
{

class Scene;

class SSAOStochasticRenderingCompositor : public kvs::StochasticRenderingCompositor
{
    using BaseClass = kvs::StochasticRenderingCompositor;

private:
    kvs::Shader::ShadingModel* m_shader = new kvs::Shader::Lambert(); ///< shader
    AmbientOcclusionBuffer m_ao_buffer{}; ///< ambient occlusion buffer

public:
    SSAOStochasticRenderingCompositor( kvs::Scene* scene ): BaseClass( scene ) {}
    virtual ~SSAOStochasticRenderingCompositor() { if ( m_shader ) delete m_shader; }

    const kvs::Shader::ShadingModel& shader() const { return *m_shader; }

    template <typename ShadingType> void setShader( const ShadingType shader )
    {
        if ( m_shader ) { delete m_shader; }
        m_shader = new ShadingType( shader );
    }

    void setKernelRadius( const float radius ) { m_ao_buffer.setKernelRadius( radius ); }
    void setKernelSize( const size_t nsamples ) { m_ao_buffer.setKernelSize( nsamples ); }
    void setDrawingOcclusionFactorEnabled( const bool enabled = true ) { m_ao_buffer.setDrawingOcclusionFactorEnabled( enabled ); }
    kvs::Real32 kernelRadius() const { return m_ao_buffer.kernelRadius(); }
    size_t kernelSize() const { return m_ao_buffer.kernelSize(); }

    KVS_DEPRECATED( void setSamplingSphereRadius( const float radius ) ) { this->setKernelRadius( radius ); }
    KVS_DEPRECATED( void setNumberOfSamplingPoints( const size_t nsamples ) ) { this->setKernelSize( nsamples ); }
    KVS_DEPRECATED( kvs::Real32 samplingSphereRadius() const ) { return this->kernelRadius(); }
    KVS_DEPRECATED( size_t numberOfSamplingPoints() const ) { return this->kernelSize(); }

protected:
    virtual void onWindowCreated();
    virtual void onWindowResized();
    virtual void updateEngines();
    virtual void setupEngines();
    virtual void ensembleRenderPass( kvs::EnsembleAverageBuffer& buffer );
};

} // end of namespace AmbientOcclusionRendering
