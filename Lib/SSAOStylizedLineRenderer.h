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
#include "SSAODrawable.h"


namespace AmbientOcclusionRendering
{

class SSAOStylizedLineRenderer : public kvs::LineRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOStylizedLineRenderer, Renderer );
    kvsModuleBaseClass( kvs::LineRenderer );

private:
    size_t m_window_width; ///< window width
    size_t m_window_height; ///< window height
    const kvs::ObjectBase* m_object; ///< pointer to the rendering object
    kvs::ValueArray<GLint> m_first_array; ///< array of starting indices for the polyline
    kvs::ValueArray<GLsizei> m_count_array; ///< array of the number of indices for the polyline
    bool m_has_connection; ///< check flag for the connection array
    kvs::Shader::ShadingModel* m_shader; ///< shading method

    // Variables for tube rendering
    kvs::Real32 m_radius_size;
    kvs::Real32 m_halo_size;
    kvs::Texture2D m_shape_texture;
    kvs::Texture2D m_diffuse_texture;
    kvs::VertexBufferObjectManager m_vbo_manager;

    SSAODrawable m_drawable;

public:
    SSAOStylizedLineRenderer();
    virtual ~SSAOStylizedLineRenderer();

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    template <typename ShadingType>
    void setShader( const ShadingType shader );
    void setRadiusSize( const kvs::Real32 size ) { m_radius_size = size; }
    void setHaloSize( const kvs::Real32 size ) { m_halo_size = size; }
    kvs::Real32 radiusSize() const { return m_radius_size; }
    kvs::Real32 haloSize() const { return m_halo_size; }
    void setSamplingSphereRadius( const float radius ) { m_drawable.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_drawable.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_drawable.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_drawable.numberOfSamplingPoints(); }

private:
    void create_buffer_object( const kvs::LineObject* line );
    void create_shape_texture();
    void create_diffuse_texture();
//    void render_geometry_pass( const kvs::LineObject* line );
//    void render_occlusion_pass();
    void draw_buffer_object( const kvs::LineObject* line );
};

template <typename ShadingType>
inline void SSAOStylizedLineRenderer::setShader( const ShadingType shader )
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
