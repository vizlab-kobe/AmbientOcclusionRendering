#include "AmbientOcclusionBuffer.h"
#include <kvs/OpenGL>
#include <kvs/ValueArray>
#include <kvs/Xorshift128>
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

namespace local
{

AmbientOcclusionBuffer::AmbientOcclusionBuffer():
    m_occl_pass_shader_vert_file("SSAO_occl_pass.vert"),
    m_occl_pass_shader_frag_file("SSAO_occl_pass.frag"),
    m_bound_id( 0 ),
    m_sampling_sphere_radius( 0.5f ),
    m_nsamples( 256 )
{
}

void AmbientOcclusionBuffer::setOcclusionPassShaderFiles( const std::string& vert_file, const std::string& frag_file )
{
    m_occl_pass_shader_vert_file = vert_file;
    m_occl_pass_shader_frag_file = frag_file;
}

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
    m_occl_pass_shader.setUniform( "color_texture", 0 );
    m_occl_pass_shader.setUniform( "position_texture", 1 );
    m_occl_pass_shader.setUniform( "normal_texture", 2 );
    m_occl_pass_shader.setUniform( "depth_texture", 3 );
    m_occl_pass_shader.setUniform( "ProjectionMatrix", kvs::OpenGL::ProjectionMatrix() );

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
}

void AmbientOcclusionBuffer::createShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled )
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

            if ( kvs::OpenGL::Boolean( GL_LIGHT_MODEL_TWO_SIDE ) == GL_TRUE )
            {
                frag.define("ENABLE_TWO_SIDE_LIGHTING");
            }
            frag.define( "NUMBER_OF_SAMPLING_POINTS " + kvs::String::ToString( m_nsamples ) );
        }

        m_occl_pass_shader.build( vert, frag );
    }

    const size_t nsamples = m_nsamples;
    const float radius = m_sampling_sphere_radius;
    const size_t dim = 3;
    const auto sampling_points = this->generatePoints( radius, nsamples );

    m_occl_pass_shader.bind();
    m_occl_pass_shader.setUniform( "shading.Ka", shading_model.Ka );
    m_occl_pass_shader.setUniform( "shading.Kd", shading_model.Kd );
    m_occl_pass_shader.setUniform( "shading.Ks", shading_model.Ks );
    m_occl_pass_shader.setUniform( "shading.S",  shading_model.S );
    m_occl_pass_shader.setUniform( "sampling_points", sampling_points, dim );
    m_occl_pass_shader.unbind();
}

void AmbientOcclusionBuffer::updateShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled )
{
    m_occl_pass_shader.release();
    this->createShaderProgram( shading_model, shading_enabled );
}

void AmbientOcclusionBuffer::setupShaderProgram(
    const kvs::Shader::ShadingModel& shading_model )
{
}

void AmbientOcclusionBuffer::createFramebuffer( const size_t width, const size_t height )
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

void AmbientOcclusionBuffer::updateFramebuffer( const size_t width, const size_t height )
{
    m_color_texture.release();
    m_color_texture.create( width, height );

    m_position_texture.release();
    m_position_texture.create( width, height );

    m_normal_texture.release();
    m_normal_texture.create( width, height );

    m_depth_texture.release();
    m_depth_texture.create( width, height );

    m_framebuffer.attachColorTexture( m_color_texture, 0 );
    m_framebuffer.attachColorTexture( m_position_texture, 1 );
    m_framebuffer.attachColorTexture( m_normal_texture, 2 );
    m_framebuffer.attachDepthTexture( m_depth_texture );
}

kvs::ValueArray<GLfloat> AmbientOcclusionBuffer::generatePoints( const float radius, const size_t nsamples )
{
    kvs::Xorshift128 rand;
    kvs::ValueArray<GLfloat> sampling_points( 3 * nsamples );
    for ( size_t i = 0; i < nsamples ; i++ )
    {
        const float pi = 3.1415926f;

        const float r = radius * rand();
        const float t = 2.0f * pi * rand();
        const float cp = 2.0f * rand() - 1.0f;
        const float sp = std::sqrt( 1.0f - cp * cp );
        const float ct = std::cos( t );
        const float st = std::sin( t );
        const float x = r * sp * ct;
        const float y = r * sp * st;
        const float z = r * cp;

        sampling_points[ 3 * i + 0 ] = x;
        sampling_points[ 3 * i + 1 ] = y;
        sampling_points[ 3 * i + 2 ] = z;
    }

    return sampling_points;
}

} // end of namespace local