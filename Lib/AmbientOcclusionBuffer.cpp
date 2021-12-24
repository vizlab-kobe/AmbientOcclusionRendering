#include "AmbientOcclusionBuffer.h"
#include <kvs/OpenGL>
#include <kvs/ValueArray>
#include <kvs/Xorshift128>
#include <kvs/MersenneTwister>
#include <cmath>


namespace
{

inline void Draw()
{
    kvs::OpenGL::WithPushedMatrix p1( GL_MODELVIEW );
    p1.loadIdentity();
    {
        kvs::OpenGL::WithPushedMatrix p2( GL_PROJECTION );
        p2.loadIdentity();
        {
            kvs::OpenGL::SetOrtho( 0, 1, 0, 1, -1, 1 );
            {
                kvs::OpenGL::Begin( GL_QUADS );
                kvs::OpenGL::Color( kvs::Vec4::Constant( 1.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 1 ), kvs::Vec2( 1, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 1 ), kvs::Vec2( 0, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 0 ), kvs::Vec2( 0, 0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 0 ), kvs::Vec2( 1, 0 ) );
                kvs::OpenGL::End();
            }
        }
    }
}

} // end of namespace

namespace AmbientOcclusionRendering
{

void AmbientOcclusionBuffer::bind()
{
    // Gaurded bind.
    m_bound_id = kvs::OpenGL::Integer( GL_FRAMEBUFFER_BINDING );
    if ( m_bound_id != m_framebuffer.id() ) { m_framebuffer.bind(); }

    // Initialize FBO.
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Enable MRT rendering.
    const GLenum buffers[3] = {
        GL_COLOR_ATTACHMENT0_EXT,
        GL_COLOR_ATTACHMENT1_EXT,
        GL_COLOR_ATTACHMENT2_EXT };
    kvs::OpenGL::SetDrawBuffers( 3, buffers );
}

void AmbientOcclusionBuffer::unbind()
{
    if ( m_bound_id != m_framebuffer.id() )
    {
        KVS_GL_CALL( glBindFramebufferEXT( GL_FRAMEBUFFER, m_bound_id ) );
    }
}

void AmbientOcclusionBuffer::draw()
{
    kvs::ProgramObject::Binder bind1( m_occl_pass_shader );
    kvs::Texture::Binder unit0( m_color_texture, 0 );
    kvs::Texture::Binder unit1( m_position_texture, 1 );
    kvs::Texture::Binder unit2( m_normal_texture, 2 );
    kvs::Texture::Binder unit3( m_depth_texture, 3 );
    kvs::Texture::Binder unit4( m_kernel_texture, 4 );
    kvs::Texture::Binder unit5( m_noise_texture, 5 );
    m_occl_pass_shader.setUniform( "color_texture", 0 );
    m_occl_pass_shader.setUniform( "position_texture", 1 );
    m_occl_pass_shader.setUniform( "normal_texture", 2 );
    m_occl_pass_shader.setUniform( "depth_texture", 3 );
    m_occl_pass_shader.setUniform( "kernel_texture", 4 );
    m_occl_pass_shader.setUniform( "noise_texture", 5 );

    const auto noise_scale = 1.0f / static_cast<float>( m_noise_size );
    m_occl_pass_shader.setUniform( "noise_scale", kvs::Vec2( noise_scale, noise_scale ) );
    m_occl_pass_shader.setUniform( "kernel_radius", m_kernel_radius );
    m_occl_pass_shader.setUniform( "kernel_bias", m_kernel_bias );
    m_occl_pass_shader.setUniform( "intensity", m_intensity );

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );
    ::Draw();
}

void AmbientOcclusionBuffer::release()
{
    // Release occl pas shader resources
    m_occl_pass_shader.release();

    // Release framebuffer resources
    m_framebuffer.release();
    m_color_texture.release();
    m_position_texture.release();
    m_normal_texture.release();
    m_depth_texture.release();

    // Release kernel texture resources
    m_kernel_texture.release();
    m_noise_texture.release();
}

void AmbientOcclusionBuffer::createShaderProgram(
    const kvs::Shader::ShadingModel& shading_model,
    const bool shading_enabled )
{
    // Build SSAO shader for occlusion-pass (2nd pass).
    {
        kvs::ShaderSource vert( m_occl_pass_shader_vert_file );
        kvs::ShaderSource frag( m_occl_pass_shader_frag_file );
        if ( shading_enabled )
        {
            switch ( shading_model.type() )
            {
            case kvs::Shader::LambertShading: frag.define("ENABLE_LAMBERT_SHADING"); break;
            case kvs::Shader::PhongShading: frag.define("ENABLE_PHONG_SHADING"); break;
            case kvs::Shader::BlinnPhongShading: frag.define("ENABLE_BLINN_PHONG_SHADING"); break;
            default: break; // NO SHADING
            }

            if ( shading_model.two_side_lighting )
            {
                frag.define("ENABLE_TWO_SIDE_LIGHTING");
            }

            if ( m_drawing_occlusion_factor )
            {
                frag.define( "ENABLE_DRAWING_OCCLUSION_FACTOR" );
            }
        }

        m_occl_pass_shader.build( vert, frag );
    }

    this->createKernelTexture( m_kernel_radius, m_kernel_size );
    m_occl_pass_shader.bind();
    m_occl_pass_shader.setUniform( "kernel_size", int( m_kernel_size ) );
    m_occl_pass_shader.unbind();
}

void AmbientOcclusionBuffer::updateShaderProgram(
    const kvs::Shader::ShadingModel& shading_model,
    const bool shading_enabled )
{
    m_occl_pass_shader.release();
    this->createShaderProgram( shading_model, shading_enabled );
}

void AmbientOcclusionBuffer::setupShaderProgram(
    const kvs::Shader::ShadingModel& shading_model )
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 P = kvs::OpenGL::ProjectionMatrix();
    const kvs::Mat4 PM = P * M;

    kvs::ProgramObject::Binder bind( m_occl_pass_shader );
    m_occl_pass_shader.setUniform( "shading.Ka", shading_model.Ka );
    m_occl_pass_shader.setUniform( "shading.Kd", shading_model.Kd );
    m_occl_pass_shader.setUniform( "shading.Ks", shading_model.Ks );
    m_occl_pass_shader.setUniform( "shading.S",  shading_model.S );
    m_occl_pass_shader.setUniform( "ModelViewProjectionMatrix", PM );
    m_occl_pass_shader.setUniform( "ProjectionMatrix", P );
}

void AmbientOcclusionBuffer::createFramebuffer(
    const size_t width,
    const size_t height )
{
    m_color_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_color_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_color_texture.setMagFilter( GL_LINEAR );
    m_color_texture.setMinFilter( GL_LINEAR );
    m_color_texture.setPixelFormat( GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE );
    m_color_texture.create( width, height );

    m_position_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_position_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_position_texture.setMagFilter( GL_LINEAR );
    m_position_texture.setMinFilter( GL_LINEAR );
    m_position_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_position_texture.create( width, height );

    m_normal_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_normal_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_normal_texture.setMagFilter( GL_LINEAR );
    m_normal_texture.setMinFilter( GL_LINEAR );
    m_normal_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_normal_texture.create( width, height );

    m_depth_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_depth_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_depth_texture.setMagFilter( GL_LINEAR );
    m_depth_texture.setMinFilter( GL_LINEAR );
    m_depth_texture.setPixelFormat( GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT  );
    m_depth_texture.create( width, height );

    m_framebuffer.create();
    m_framebuffer.attachColorTexture( m_color_texture, 0 );
    m_framebuffer.attachColorTexture( m_position_texture, 1 );
    m_framebuffer.attachColorTexture( m_normal_texture, 2 );
    m_framebuffer.attachDepthTexture( m_depth_texture );
}

void AmbientOcclusionBuffer::updateFramebuffer(
    const size_t width,
    const size_t height )
{
    m_color_texture.release();
    m_position_texture.release();
    m_normal_texture.release();
    m_depth_texture.release();
    m_framebuffer.release();
    this->createFramebuffer( width, height );
}

void AmbientOcclusionBuffer::createKernelTexture(
    const float radius,
    const size_t nsamples )
{
    m_kernel_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_kernel_texture.setMagFilter( GL_NEAREST );
    m_kernel_texture.setMinFilter( GL_NEAREST );
    m_kernel_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGB, GL_FLOAT );

    auto samples = this->generatePoints( radius, nsamples );
    m_kernel_texture.create( nsamples, samples.data() );

    m_noise_texture.setWrapS( GL_REPEAT );
    m_noise_texture.setWrapS( GL_REPEAT );
    m_noise_texture.setMagFilter( GL_NEAREST );
    m_noise_texture.setMinFilter( GL_NEAREST );
    m_noise_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGB, GL_FLOAT );

    auto noises = this->generateNoises( m_noise_size );
    m_noise_texture.create( m_noise_size, m_noise_size, noises.data() );
}

void AmbientOcclusionBuffer::updateKernelTexture(
    const float radius,
    const size_t nsamples )
{
    m_kernel_texture.release();
    m_noise_texture.release();
    this->createKernelTexture( radius, nsamples );
}

kvs::ValueArray<GLfloat> AmbientOcclusionBuffer::generatePoints(
    const float radius,
    const size_t nsamples )
{
    auto lerp = [] ( float a, float b, float f ) { return a + f * ( b - 1 ); };

    kvs::MersenneTwister rand( nsamples );
    kvs::ValueArray<GLfloat> sampling_points( 3 * nsamples );
    for ( size_t i = 0; i < nsamples ; ++i )
    {
        kvs::Vec3 sample(
            rand() * 2.0f - 1.0f, // in [-1.0, 1.0]
            rand() * 2.0f - 1.0f, // in [-1.0, 1.0]
            rand() );             // in [ 0.0, 1.0]
        sample.normalize();
        sample *= rand();

        float scale = i / static_cast<float>( nsamples );
        scale = lerp( 0.1f, 1.0f, scale * scale );
        sample *= scale;

        sampling_points[ 3 * i + 0 ] = sample.x();
        sampling_points[ 3 * i + 1 ] = sample.y();
        sampling_points[ 3 * i + 2 ] = sample.z();
    }

    return sampling_points;
}

kvs::ValueArray<GLfloat> AmbientOcclusionBuffer::generateNoises( const size_t noise_size )
{
    kvs::MersenneTwister rand( noise_size );
    const size_t size = m_noise_size * m_noise_size;
    kvs::ValueArray<GLfloat> noises( size * 3 );
    for ( size_t i = 0; i < size; ++i )
    {
        kvs::Vec3 noise(
            rand() * 2.0f - 1.0f, // in [-1.0, 1.0]
            rand() * 2.0f - 1.0f, // in [-1.0, 1.0]
            0 );
        noise.normalize();
        noises[ 3 * i + 0 ] = noise.x();
        noises[ 3 * i + 1 ] = noise.y();
        noises[ 3 * i + 2 ] = noise.z();
    }
    return noises;
}

} // end of namespace AmbientOcclusionRendering
