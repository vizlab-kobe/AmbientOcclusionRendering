/*****************************************************************************/
/**
 *  @file   SSAOPolygonRenderer.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/DebugNew>
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include "AmbientOcclusionBuffer.h"


namespace AmbientOcclusionRendering
{

class SSAOPolygonRenderer : public kvs::glsl::PolygonRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOPolygonRenderer, Renderer );
    kvsModuleBaseClass( kvs::glsl::PolygonRenderer );

private:
    AmbientOcclusionBuffer m_ao_buffer;

public:
    SSAOPolygonRenderer();
    virtual ~SSAOPolygonRenderer() {}

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }

private:
    void createShaderProgram();
    void updateShaderProgram();
    void setupShaderProgram();

    void createFramebuffer( const size_t width, const size_t height );
    void updateFramebuffer( const size_t width, const size_t height );
};

} // end of namespace AmbientOcclusionRendering
