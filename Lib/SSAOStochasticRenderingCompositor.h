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

    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }
    const kvs::Shader::ShadingModel& shader() const { return *m_shader; }

    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    template <typename ShadingType> void setShader( const ShadingType shader )
    {
        if ( m_shader ) { delete m_shader; }
        m_shader = new ShadingType( shader );
    }

protected:
    virtual void onWindowCreated();
    virtual void onWindowResized();
    virtual void updateEngines();
    virtual void setupEngines();
    virtual void bindBuffer();
    virtual void unbindBuffer();
};

} // end of namespace AmbientOcclusionRendering
