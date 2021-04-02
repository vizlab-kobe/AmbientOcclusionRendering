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
 *  @brief  Ambient occlusion buffer class.
 */
/*===========================================================================*/
class AmbientOcclusionBuffer
{
private:
    // Occlusion pass shader
    std::string m_occl_pass_shader_vert_file = "SSAO_occl_pass.vert"; ///< vertex shader file for occlusion pass
    std::string m_occl_pass_shader_frag_file = "SSAO_occl_pass.frag"; ///< fragment shader file for occlusion pass
    kvs::ProgramObject m_occl_pass_shader{}; ///< shader program for occlusion-pass (2nd pass)

    // Framebuffer for SSAO
    GLuint m_bound_id = 0; ///< Bound framebuffer ID
    kvs::FrameBufferObject m_framebuffer{}; ///< framebuffer object
    kvs::Texture2D m_color_texture{}; ///< color texture
    kvs::Texture2D m_position_texture{}; ///< texture for storing position information
    kvs::Texture2D m_normal_texture{}; ///< texture for storing normal vector
    kvs::Texture2D m_depth_texture{}; ///< depth texture

    // Sampling point parameters
    kvs::Real32 m_sampling_sphere_radius = 0.5f; ///< radius of sphere used for point sampling
    size_t m_nsamples = 256; ///< number of sampling points

public:
    AmbientOcclusionBuffer() = default;
    virtual ~AmbientOcclusionBuffer() { this->release(); }

    void setOcclusionPassShaderFiles(
        const std::string& vert_file,
        const std::string& frag_file )
    {
        m_occl_pass_shader_vert_file = vert_file;
        m_occl_pass_shader_frag_file = frag_file;
    }

    void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }

    const std::string& occlusionPassVertexShaderFile() const { return m_occl_pass_shader_vert_file; }
    const std::string& occlusionPassFragmentShaderFile() const { return m_occl_pass_shader_frag_file; }
    kvs::ProgramObject& occlusionPassShader() { return m_occl_pass_shader; }

    kvs::FrameBufferObject& framebuffer() { return m_framebuffer; }
    kvs::Texture2D& colorTexture() { return m_color_texture; }
    kvs::Texture2D& positionTexture() { return m_position_texture; }
    kvs::Texture2D& normalTexture() { return m_normal_texture; }
    kvs::Texture2D& depthTexture() { return m_depth_texture; }

    kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
    size_t numberOfSamplingPoints() const { return m_nsamples; }

    void bind();
    void unbind();
    void draw();
    void release();

    void createShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void updateShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void setupShaderProgram( const kvs::Shader::ShadingModel& shading_model );
    void createFramebuffer( const size_t width, const size_t height );
    void updateFramebuffer( const size_t width, const size_t height );
    void renderOcclusionPass() { this->draw(); }

    kvs::ValueArray<GLfloat> generatePoints( const float radius, const size_t nsamples );
};


namespace Deprecated
{

/*===========================================================================*/
/**
 *  @brief  Ambient occlusion buffer class
 */
/*===========================================================================*/
class AmbientOcclusionBuffer
{
private:
    // Geometry pass shader
    std::string m_geom_pass_shader_vert_file = "SSAO_geom_pass.vert"; ///< vertex shader file for geometry pass
    std::string m_geom_pass_shader_frag_file = "SSAO_geom_pass.frag"; ///< fragment shader file for geometry pass
    kvs::ProgramObject m_geom_pass_shader{}; ///< shader program for geometry-pass (1st pass)

    // Occlusion pass shader
    std::string m_occl_pass_shader_vert_file = "SSAO_occl_pass.vert"; ///< vertex shader file for occlusion pass
    std::string m_occl_pass_shader_frag_file = "SSAO_occl_pass.frag"; ///< fragment shader file for occlusion pass
    kvs::ProgramObject m_occl_pass_shader{}; ///< shader program for occlusion-pass (2nd pass)

    // Framebuffer for AO
    GLuint m_bound_id = 0; ///< Bound framebuffer ID
    kvs::FrameBufferObject m_framebuffer{}; ///< framebuffer object
    kvs::Texture2D m_color_texture{}; ///< color texture
    kvs::Texture2D m_position_texture{}; ///< texture for storing position information
    kvs::Texture2D m_normal_texture{}; ///< texture for storing normal vector
    kvs::Texture2D m_depth_texture{}; ///< depth texture

    // Sampling point parameters
    kvs::Real32 m_sampling_sphere_radius = 0.5f; ///< radius of sphere used for point sampling
    size_t m_nsamples = 256; ///< number of sampling points

public:
    AmbientOcclusionBuffer() = default;
    virtual ~AmbientOcclusionBuffer() { this->release(); }

    void setGeometryPassShaderFiles(
        const std::string& vert_file,
        const std::string& frag_file )
    {
        m_geom_pass_shader_vert_file = vert_file;
        m_geom_pass_shader_frag_file = frag_file;
    }

    void setOcclusionPassShaderFiles(
        const std::string& vert_file,
        const std::string& frag_file )
    {
        m_occl_pass_shader_vert_file = vert_file;
        m_occl_pass_shader_frag_file = frag_file;
    }

    void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }

    const std::string& geometryPassVertexShaderFile() const { return m_geom_pass_shader_vert_file; }
    const std::string& geometryPassFragmentShaderFile() const { return m_geom_pass_shader_frag_file; }
    kvs::ProgramObject& geometryPassShader() { return m_geom_pass_shader; }

    const std::string& occlusionPassVertexShaderFile() const { return m_occl_pass_shader_vert_file; }
    const std::string& occlusionPassFragmentShaderFile() const { return m_occl_pass_shader_frag_file; }
    kvs::ProgramObject& occlusionPassShader() { return m_occl_pass_shader; }

    kvs::FrameBufferObject& framebuffer() { return m_framebuffer; }
    kvs::Texture2D& colorTexture() { return m_color_texture; }
    kvs::Texture2D& positionTexture() { return m_position_texture; }
    kvs::Texture2D& normalTexture() { return m_normal_texture; }
    kvs::Texture2D& depthTexture() { return m_depth_texture; }

    kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
    size_t numberOfSamplingPoints() const { return m_nsamples; }

    void bind();
    void unbind();
    void draw();
    void release();

    void createShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void updateShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void setupShaderProgram( const kvs::Shader::ShadingModel& shading_model );
    void createFramebuffer( const size_t width, const size_t height );
    void updateFramebuffer( const size_t width, const size_t height );
    void renderOcclusionPass() { this->draw(); }

    kvs::ValueArray<GLfloat> generatePoints( const float radius, const size_t nsamples );
};

} // end of namespace Deprecated

} // end of namespace AmbientOcclusionRendering
