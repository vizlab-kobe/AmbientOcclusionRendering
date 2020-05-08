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
    kvs::ProgramObject m_shader_geom_pass; ///< shader program for geometry-pass (1st pass)
    kvs::ProgramObject m_shader_occl_pass; ///< shader program for occlusion-pass (2nd pass)

    // Variables for tube rendering
    kvs::Real32 m_radius_size;
    kvs::Real32 m_halo_size;
    kvs::Texture2D m_shape_texture;
    kvs::Texture2D m_diffuse_texture;
    kvs::VertexBufferObjectManager m_vbo_manager;

    // Variables for SSAO
    kvs::FrameBufferObject m_framebuffer;
    kvs::Texture2D m_color_texture;
    kvs::Texture2D m_position_texture;
    kvs::Texture2D m_normal_texture;
    kvs::Texture2D m_depth_texture;
    kvs::Real32 m_sampling_sphere_radius;
    size_t m_nsamples;

public:
    SSAOStylizedLineRenderer();
    virtual ~SSAOStylizedLineRenderer();

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    template <typename ShadingType>
    void setShader( const ShadingType shader );
    void setRadiusSize( const kvs::Real32 size ) { m_radius_size = size; }
    void setHaloSize( const kvs::Real32 size ) { m_halo_size = size; }
    void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }
    kvs::Real32 radiusSize() const { return m_radius_size; }
    kvs::Real32 haloSize() const { return m_halo_size; }
    kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
    size_t numberOfSamplingPoints() const { return m_nsamples; }

private:
    void create_shader_program();
    void create_buffer_object( const kvs::LineObject* line );
    void create_shape_texture();
    void create_diffuse_texture();
    void create_sampling_points();
    void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );
    void render_geometry_pass( const kvs::LineObject* line );
    void render_occlusion_pass();
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
