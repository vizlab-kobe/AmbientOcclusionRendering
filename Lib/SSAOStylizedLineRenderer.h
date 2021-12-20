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
#include <kvs/Deprecated>
#include "AmbientOcclusionBuffer.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  SSAO stochastic polygon renderer class.
 */
/*===========================================================================*/
class SSAOStylizedLineRenderer : public kvs::StylizedLineRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOStylizedLineRenderer, Renderer );
    kvsModuleBaseClass( kvs::StylizedLineRenderer );

private:
    AmbientOcclusionBuffer m_ao_buffer; ///< ambient occlusion buffer

public:
    SSAOStylizedLineRenderer();
    virtual ~SSAOStylizedLineRenderer() { m_ao_buffer.release(); }

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setKernelRadius( const float radius ) { m_ao_buffer.setKernelRadius( radius ); }
    void setKernelSize( const size_t nsamples ) { m_ao_buffer.setKernelSize( nsamples ); }
    void setDrawingOcclusionFactorEnabled( const bool enabled = true ) { m_ao_buffer.setDrawingOcclusionFactorEnabled( enabled ); }
    kvs::Real32 kernelRadius() const { return m_ao_buffer.kernelRadius(); }
    size_t kernelSize() const { return m_ao_buffer.kernelSize(); }

    KVS_DEPRECATED( void setSamplingSphereRadius( const float radius ) ) { this->setKernelRadius( radius ); }
    KVS_DEPRECATED( void setNumberOfSamplingPoints( const size_t nsamples ) ) { this->setKernelSize( nsamples ); }
    KVS_DEPRECATED( kvs::Real32 samplingSphereRadius() const ) { return this->kernelRadius(); }
    KVS_DEPRECATED( size_t numberOfSamplingPoints() const ) { return this->kernelSize(); }

private:
    void create_shader_program();
    void update_shader_program();
    void setup_shader_program();

    void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );
};

} // end of namespace AmbientOcclusionRendering
