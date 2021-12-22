#pragma once
#include <string>
#include <kvs/ProgramObject>
#include <kvs/FrameBufferObject>
#include <kvs/Texture2D>
#include <kvs/Shader>
#include <kvs/Deprecated>


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

    // Sampling kernel
    kvs::Real32 m_kernel_radius = 0.5f; ///< radius of kernel sphere used for point sampling
    size_t m_kernel_size = 256; ///< number of sampling points
    float m_kernel_bias = 0.0f; ///< tolerance factor for depth comparison
    kvs::Texture1D m_kernel_texture{}; ///< sampling point texture
    float m_intensity = 1.0f; ///< occlusion intensity
    size_t m_noise_size = 4; ///< noise texture size: m_noise_size x m_noise_size
    kvs::Texture2D m_noise_texture{}; ///< noise texture used to rotate the kernel

    bool m_drawing_occlusion_factor = false; ///< flag for drawing occlusion factor

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

    void setKernelRadius( const kvs::Real32 radius ) { m_kernel_radius = radius; }
    void setKernelSize( const size_t nsamples ) { m_kernel_size = nsamples; }
    void setKernelBias( const float bias ) { m_kernel_bias = bias; }
    void setIntensity( const float intensity ) { m_intensity = intensity; }
    void setDrawingOcclusionFactorEnabled( const bool enabled = true ) { m_drawing_occlusion_factor = enabled; }

    const std::string& occlusionPassVertexShaderFile() const { return m_occl_pass_shader_vert_file; }
    const std::string& occlusionPassFragmentShaderFile() const { return m_occl_pass_shader_frag_file; }
    kvs::ProgramObject& occlusionPassShader() { return m_occl_pass_shader; }

    kvs::FrameBufferObject& framebuffer() { return m_framebuffer; }
    kvs::Texture2D& colorTexture() { return m_color_texture; }
    kvs::Texture2D& positionTexture() { return m_position_texture; }
    kvs::Texture2D& normalTexture() { return m_normal_texture; }
    kvs::Texture2D& depthTexture() { return m_depth_texture; }

    kvs::Real32 kernelRadius() const { return m_kernel_radius; }
    size_t kernelSize() const { return m_kernel_size; }
    float kernelBias() const { return m_kernel_bias; }
    float intensity() const { return m_intensity; }

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

    void createKernelTexture( const float radius, const size_t nsamples );
    void updateKernelTexture( const float radius, const size_t nsamples );
    kvs::ValueArray<GLfloat> generatePoints( const float radius, const size_t nsamples );
    kvs::ValueArray<GLfloat> generateNoises( const size_t noise_size );

    KVS_DEPRECATED( void setSamplingSphereRadius( const kvs::Real32 radius ) ) { this->setKernelRadius( radius ); }
    KVS_DEPRECATED( void setNumberOfSamplingPoints( const size_t nsamples ) ) { this->setKernelSize( nsamples ); }
    KVS_DEPRECATED( kvs::Real32 samplingSphereRadius() const ) { return this->kernelRadius(); }
    KVS_DEPRECATED( size_t numberOfSamplingPoints() const ) { return this->kernelSize(); }
};

} // end of namespace AmbientOcclusionRendering
