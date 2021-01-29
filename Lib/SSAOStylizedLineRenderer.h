#pragma once
#include <kvs/DebugNew>
#include <kvs/Module>
#include <kvs/LineObject>
#include <kvs/LineRenderer>
#include <kvs/Shader>
#include <kvs/ProgramObject>
#include <kvs/FrameBufferObject>
#include <kvs/Texture2D>
#include <kvs/VertexBufferObjectManager>
#include <kvs/StylizedLineRenderer>
#include "AmbientOcclusionBuffer.h"


namespace AmbientOcclusionRendering
{

class SSAOStylizedLineRenderer : public kvs::StylizedLineRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOStylizedLineRenderer, Renderer );
    kvsModuleBaseClass( kvs::StylizedLineRenderer );

private:
    AmbientOcclusionBuffer m_ao_buffer;

public:
    SSAOStylizedLineRenderer();
    virtual ~SSAOStylizedLineRenderer() {}

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
