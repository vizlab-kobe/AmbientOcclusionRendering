#include "SSAOFrameBuffer.h"
#include "SSAOPointSampling.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/String>


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
                kvs::OpenGL::Color( kvs::Vec4::All( 1.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 1 ), kvs::Vec2( 1, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 1 ), kvs::Vec2( 0, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 0 ), kvs::Vec2( 0, 0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 0 ), kvs::Vec2( 1, 0 ) );
                kvs::OpenGL::End();
            }
        }
    }
}

}

namespace AmbientOcclusionRendering
{

SSAOFrameBuffer::SSAOFrameBuffer():
    m_shader( new kvs::Shader::Lambert() ),
    m_enable_shading( true )
{
}

/*===========================================================================*/
/**
 *  @brief  Creates the buffers.
 *  @param  width [in] buffer width
 *  @param  height [in] buffer height
 */
/*===========================================================================*/
void SSAOFrameBuffer::create( const size_t width, const size_t height, kvs::Real32 radius, const size_t nsamples)
{
    this->create_shader_program( nsamples );
    this->create_sampling_points( radius, nsamples );
    this->create_framebuffer( width, height );
    
}

/*===========================================================================*/
/**
 *  @brief  Release the buffer resources.
 */
/*===========================================================================*/
void SSAOFrameBuffer::release()
{
    m_color_texture.release();
    m_position_texture.release();
    m_normal_texture.release();
    m_depth_texture.release();
    m_framebuffer.release();
    m_shader_occl_pass.release();
}

/*===========================================================================*/
/**
 *  @brief  Clear the accumulation buffer.
 */
/*===========================================================================*/
void SSAOFrameBuffer::clear()
{
    kvs::FrameBufferObject::Binder binder( m_framebuffer );
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

/*===========================================================================*/
/**
 *  @brief  Bind the current buffer.
 */
/*===========================================================================*/
void SSAOFrameBuffer::bind()
{
    m_id = kvs::OpenGL::Integer( GL_FRAMEBUFFER_BINDING );
    m_framebuffer.bind();
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

     // Enable MRT rendering.
    const GLenum buffers[3] = {
        GL_COLOR_ATTACHMENT0_EXT,
        GL_COLOR_ATTACHMENT1_EXT,
        GL_COLOR_ATTACHMENT2_EXT };
    kvs::OpenGL::SetDrawBuffers( 3, buffers );
}

/*===========================================================================*/
/**
 *  @brief  Unbind the current buffer.
 */
/*===========================================================================*/
void SSAOFrameBuffer::unbind()
{
    KVS_GL_CALL( glBindFramebufferEXT( GL_FRAMEBUFFER, m_id ) );
}

/*===========================================================================*/
/**
 *  @brief  Draw the accumulation buffer.
 */
/*===========================================================================*/
void SSAOFrameBuffer::draw()
{
    kvs::ProgramObject::Binder bind1( m_shader_occl_pass );
    kvs::Texture::Binder unit0( m_color_texture, 0 );
    kvs::Texture::Binder unit1( m_position_texture, 1 );
    kvs::Texture::Binder unit2( m_normal_texture, 2 );
    kvs::Texture::Binder unit3( m_depth_texture, 3 );
    m_shader_occl_pass.setUniform( "color_texture", 0 );
    m_shader_occl_pass.setUniform( "position_texture", 1 );
    m_shader_occl_pass.setUniform( "normal_texture", 2 );
    m_shader_occl_pass.setUniform( "depth_texture", 3 );
    m_shader_occl_pass.setUniform( "ProjectionMatrix", kvs::OpenGL::ProjectionMatrix() );

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );
    ::Draw();
}

void SSAOFrameBuffer::update_framebuffer( const size_t width, const size_t height )
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

void SSAOFrameBuffer::create_sampling_points( kvs::Real32 sphere_radius, const size_t nsamples )
{
    //const size_t nsamples = nsamples;
    const float radius = sphere_radius;
    const size_t dim = 3;
    const kvs::ValueArray<GLfloat> sampling_points = AmbientOcclusionRendering::SSAOPointSampling( radius, nsamples );
    m_shader_occl_pass.bind();
    m_shader_occl_pass.setUniform( "sampling_points", sampling_points, dim );
    m_shader_occl_pass.unbind();
}

void SSAOFrameBuffer::create_shader_program( const size_t nsamples )
{
    // Build SSAO shader for occlusion-pass (2nd pass).
    kvs::ShaderSource vert( "SSAO_occl_pass.vert" );
    kvs::ShaderSource frag( "SSAO_occl_pass.frag" );
    if ( m_enable_shading )
    {
        switch ( shader().type() )
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

        frag.define( "NUMBER_OF_SAMPLING_POINTS " + kvs::String::ToString( nsamples ) );
    }

    m_shader_occl_pass.build( vert, frag );
    m_shader_occl_pass.bind();
    m_shader_occl_pass.setUniform( "shading.Ka", shader().Ka );
    m_shader_occl_pass.setUniform( "shading.Kd", shader().Kd );
    m_shader_occl_pass.setUniform( "shading.Ks", shader().Ks );
    m_shader_occl_pass.setUniform( "shading.S",  shader().S );
    m_shader_occl_pass.unbind();    
}

void SSAOFrameBuffer::create_framebuffer( const size_t width, const size_t height )
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
/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOFrameBuffer::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    this->update_framebuffer( camera->windowWidth(), camera->windowHeight() );
}

/*kvs::Real32 SSAOFrameBuffer::samplingSphereRadius()
{
    return m_sampling_sphere_radius;
    }

size_t SSAOFrameBuffer::numberOfSamplingPoints()
{
    return m_nsamples;
}
*/

/*bool SSAOFrameBuffer::isEnabledShading()
{
    return m_enable_shading;
    }*/

} // end of namespace kvs
