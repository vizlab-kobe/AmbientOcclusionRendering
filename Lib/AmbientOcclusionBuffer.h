#pragma once
#include <string>
#include <kvs/ProgramObject>
#include <kvs/FrameBufferObject>
#include <kvs/Texture2D>
#include <kvs/Shader>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Drawable class for SSAO
 */
/*===========================================================================*/
class AmbientOcclusionBuffer
{
private:
    GLuint m_id;

    std::string m_geom_pass_shader_vert_file; ///< vertex shader file for geometry pass
    std::string m_geom_pass_shader_frag_file; ///< fragment shader file for geometry pass
    kvs::ProgramObject m_geom_pass_shader; ///< shader program for geometry-pass (1st pass)

    std::string m_occl_pass_shader_vert_file; ///< vertex shader file for occlusion pass
    std::string m_occl_pass_shader_frag_file; ///< fragment shader file for occlusion pass
    kvs::ProgramObject m_occl_pass_shader; ///< shader program for occlusion-pass (2nd pass)
    kvs::FrameBufferObject m_framebuffer; ///< framebuffer object
    kvs::Texture2D m_color_texture; ///< color texture
    kvs::Texture2D m_position_texture; ///< texture for storing position information
    kvs::Texture2D m_normal_texture; ///< texture for storing normal vector
    kvs::Texture2D m_depth_texture; ///< depth texture
    kvs::Real32 m_sampling_sphere_radius; ///< radius of sphere used for point sampling
    size_t m_nsamples; ///< number of sampling points

public:
    AmbientOcclusionBuffer();
    virtual ~AmbientOcclusionBuffer() {}

    void setGeometryPassShaderFiles( const std::string& vert_file, const std::string& frag_file );
    const std::string& geometryPassVertexShaderFile() const { return m_geom_pass_shader_vert_file; }
    const std::string& geometryPassFragmentShaderFile() const { return m_geom_pass_shader_frag_file; }
    kvs::ProgramObject& geometryPassShader() { return m_geom_pass_shader; }

    void setOcclusionPassShaderFiles( const std::string& vert_file, const std::string& frag_file );
    void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }

    const std::string& occlusionPassVertexShaderFile() const { return m_occl_pass_shader_vert_file; }
    const std::string& occlusionPassFragmentShaderFile() const { return m_occl_pass_shader_frag_file; }
    kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
    size_t numberOfSamplingPoints() const { return m_nsamples; }

    kvs::ProgramObject& occlusionPassShader() { return m_occl_pass_shader; }
    kvs::FrameBufferObject& framebuffer() { return m_framebuffer; }
    kvs::Texture2D& colorTexture() { return m_color_texture; }
    kvs::Texture2D& positionTexture() { return m_position_texture; }
    kvs::Texture2D& normalTexture() { return m_normal_texture; }
    kvs::Texture2D& depthTexture() { return m_depth_texture; }

    void bind();
    void unbind();
    void draw();

    void releaseResources();
    void createShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void updateShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void createFramebuffer( const size_t width, const size_t height );
    void updateFramebuffer( const size_t width, const size_t height );
    void renderOcclusionPass() { this->draw(); }

    kvs::ValueArray<GLfloat> generatePoints( const float radius, const size_t nsamples );
};

} // end of namespace AmbientOcclusionRendering
