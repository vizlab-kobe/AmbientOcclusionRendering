#include "SSAOStochasticStylizedLineRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/ShaderSource>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/Xorshift128>
#include <kvs/String>


namespace
{

/*===========================================================================*/
/**
 *  @brief  Returns a random number as integer value.
 *  @return random number
 */
/*===========================================================================*/
inline int RandomNumber()
{
    const int C = 12347;
    static kvs::Xorshift128 R;
    return C * R.randInteger();
}

}

namespace AmbientOcclusionRendering
{

SSAOStochasticStylizedLineRenderer::SSAOStochasticStylizedLineRenderer():
    StochasticRendererBase( new Engine() )
{
}

void SSAOStochasticStylizedLineRenderer::setOpacity( const kvs::UInt8 opacity )
{
    static_cast<Engine&>( engine() ).setOpacity( opacity );
}

void SSAOStochasticStylizedLineRenderer::setRadiusSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setRadiusSize( size );
}

void SSAOStochasticStylizedLineRenderer::setHaloSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setHaloSize( size );
}

void SSAOStochasticStylizedLineRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

void SSAOStochasticStylizedLineRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

kvs::UInt8 SSAOStochasticStylizedLineRenderer::opacity() const
{
    return static_cast<const Engine&>( engine() ).opacity();
}

kvs::Real32 SSAOStochasticStylizedLineRenderer::radiusSize() const
{
    return static_cast<const Engine&>( engine() ).radiusSize();
}

kvs::Real32 SSAOStochasticStylizedLineRenderer::haloSize() const
{
    return static_cast<const Engine&>( engine() ).haloSize();
}

kvs::Real32 SSAOStochasticStylizedLineRenderer::samplingSphereRadius()
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

size_t SSAOStochasticStylizedLineRenderer::numberOfSamplingPoints()
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

SSAOStochasticStylizedLineRenderer::Engine::Engine():
    m_line_opacity( 255 ),
    m_radius_size( 0.05f ),
    m_halo_size( 0.0f )
{
    m_ao_buffer.setGeometryPassShaderFiles( "SSAO_SR_stylized_geom_pass.vert", "SSAO_SR_stylized_geom_pass.frag" );
    m_ao_buffer.setOcclusionPassShaderFiles( "SSAO_occl_pass.vert", "SSAO_occl_pass.frag" );
}

void SSAOStochasticStylizedLineRenderer::Engine::release()
{
    m_buffer_object.release();
    m_ao_buffer.release();
}

void SSAOStochasticStylizedLineRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::LineObject* line = kvs::LineObject::DownCast( object );
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );

    attachObject( line );
    createRandomTexture();
    m_ao_buffer.createShaderProgram( this->shader(), this->isEnabledShading() );
    m_ao_buffer.createFramebuffer( framebuffer_width, framebuffer_height );
    this->create_buffer_object( line );
}

void SSAOStochasticStylizedLineRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    m_ao_buffer.updateFramebuffer( framebuffer_width, framebuffer_height );
}

void SSAOStochasticStylizedLineRenderer::Engine::setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 P = kvs::OpenGL::ProjectionMatrix();
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    m_ao_buffer.geometryPassShader().bind();
    m_ao_buffer.geometryPassShader().setUniform( "ModelViewMatrix", M );
    m_ao_buffer.geometryPassShader().setUniform( "ProjectionMatrix", P );
    m_ao_buffer.geometryPassShader().setUniform( "NormalMatrix", N );
    m_ao_buffer.geometryPassShader().setUniform( "shape_texture", 0 );
    m_ao_buffer.geometryPassShader().setUniform( "diffuse_texture", 1 );
    m_ao_buffer.geometryPassShader().setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
    m_ao_buffer.geometryPassShader().setUniform( "random_texture", 2 );
    m_ao_buffer.geometryPassShader().setUniform( "opacity", m_line_opacity / 255.0f );
    m_ao_buffer.geometryPassShader().unbind();
}

void SSAOStochasticStylizedLineRenderer::Engine::draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    m_ao_buffer.bind();
    this->draw_buffer_object( kvs::LineObject::DownCast( object ) );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();
}

void SSAOStochasticStylizedLineRenderer::Engine::create_buffer_object( const kvs::LineObject* line )
{
    const size_t nvertices = line->numberOfVertices() * 2;
    const auto tex_size = randomTextureSize();
    kvs::ValueArray<kvs::UInt16> indices( nvertices * 2 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        const unsigned int count = i * 12347;
        indices[ 2 * i + 0 ] = static_cast<kvs::UInt16>( ( count ) % tex_size );
        indices[ 2 * i + 1 ] = static_cast<kvs::UInt16>( ( count / tex_size ) % tex_size );
    }

    auto location = m_ao_buffer.geometryPassShader().attributeLocation( "random_index" );
    m_buffer_object.manager().setVertexAttribArray( indices, location, 2 );
    m_buffer_object.create( line, m_halo_size, m_radius_size );
}

void SSAOStochasticStylizedLineRenderer::Engine::draw_buffer_object( const kvs::LineObject* line )
{
    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );

    const size_t size = randomTextureSize();
    const int count = repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );
    m_ao_buffer.geometryPassShader().setUniform( "random_offset", random_offset );

    kvs::Texture::Binder unit2( randomTexture(), 2 );
    m_buffer_object.draw( line );
}

} // end of namespace AmbientOcclusionRendering
