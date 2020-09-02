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
#include <kvs/Shader>
#include <kvs/ProgramObject>
#include <kvs/FrameBufferObject>
#include <kvs/Texture2D>
#include <kvs/VertexBufferObjectManager>
#include "SSAODrawable.h"


namespace AmbientOcclusionRendering
{

class SSAOPolygonRenderer : public kvs::PolygonRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOPolygonRenderer, Renderer );
    kvsModuleBaseClass( kvs::PolygonRenderer );

private:
    size_t m_window_width; ///< window width
    size_t m_window_height; ///< window height
    const kvs::ObjectBase* m_object; ///< pointer to the rendering object
    bool m_has_normal; ///< check flag for the normal array
    bool m_has_connection; ///< check flag for the connection array
    kvs::Shader::ShadingModel* m_shader; ///< shading method
    kvs::VertexBufferObjectManager m_vbo_manager;
    SSAODrawable m_drawable;

public:
    SSAOPolygonRenderer();
    virtual ~SSAOPolygonRenderer();

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    template <typename ShadingType>
    void setShader( const ShadingType shader );
    void setSamplingSphereRadius( const float radius ) { m_drawable.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_drawable.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_drawable.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_drawable.numberOfSamplingPoints(); }

private:
    void create_buffer_object( const kvs::PolygonObject* point );
    void render_geometry_pass( const kvs::PolygonObject* polygon );
    void render_occlusion_pass();
};

template <typename ShadingType>
inline void SSAOPolygonRenderer::setShader( const ShadingType shader )
{
    if ( m_shader )
    {
        delete m_shader;
        m_shader = NULL;
    }

    m_shader = new ShadingType( shader );
    if ( !m_shader )
    {
        kvsMessageError("Cannot create a specified shader.");
    }
};

} // end of namespace AmbientOcclusionRendering
