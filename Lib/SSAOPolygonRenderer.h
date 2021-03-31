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

/*===========================================================================*/
/**
 *  @brief  SSAO polygon renderer class.
 */
/*===========================================================================*/
class SSAOPolygonRenderer : public kvs::glsl::PolygonRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOPolygonRenderer, Renderer );
    kvsModuleBaseClass( kvs::glsl::PolygonRenderer );

private:
    AmbientOcclusionBuffer m_ao_buffer; ///< ambient occlusion buffer

public:
    SSAOPolygonRenderer();
    virtual ~SSAOPolygonRenderer() { m_ao_buffer.release(); }

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }

private:
    void create_shader_program();
    void update_shader_program();
    void setup_shader_program();

    void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );

    void draw_buffer_object( const kvs::PolygonObject* polygon );
};

} // end of namespace AmbientOcclusionRendering
