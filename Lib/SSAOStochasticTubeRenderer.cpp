#include "SSAOStochasticTubeRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/ShaderSource>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/Xorshift128>
#include <kvs/String>
#include <kvs/IgnoreUnusedVariable>


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

inline kvs::ValueArray<kvs::Real32> QuadVertexValues( const kvs::LineObject* line )
{
    const size_t nvertices = line->numberOfVertices();
    kvs::ValueArray<kvs::Real32> values( nvertices * 2 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        values[ 2 * i + 0 ] = line->sizes()[i];
        values[ 2 * i + 1 ] = line->sizes()[i];
    }
    return values;
}

}

namespace AmbientOcclusionRendering
{

SSAOStochasticTubeRenderer::SSAOStochasticTubeRenderer():
    SSAOStochasticRendererBase( new Engine() )
{
}

void SSAOStochasticTubeRenderer::setEdgeFactor( const float factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( factor );
}

void SSAOStochasticTubeRenderer::setTransferFunction( const kvs::TransferFunction& tfunc )
{
    static_cast<Engine&>( engine() ).setTransferFunction( tfunc );
}

void SSAOStochasticTubeRenderer::setRadiusSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setRadiusSize( size );
}

void SSAOStochasticTubeRenderer::setHaloSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setHaloSize( size );
}

const kvs::TransferFunction& SSAOStochasticTubeRenderer::transferFunction() const
{
    return static_cast<const Engine&>( engine() ).transferFunction();
}

kvs::Real32 SSAOStochasticTubeRenderer::radiusSize() const
{
    return static_cast<const Engine&>( engine() ).radiusSize();
}

kvs::Real32 SSAOStochasticTubeRenderer::haloSize() const
{
    return static_cast<const Engine&>( engine() ).haloSize();
}

SSAOStochasticTubeRenderer::Engine::Engine()
{
    m_render_pass.setShaderFiles(
        "SSAO_SR_tube_geom_pass.vert",
        "SSAO_SR_tube_geom_pass.frag" );
}

void SSAOStochasticTubeRenderer::Engine::release()
{
    m_buffer_object.release();
    m_render_pass.release();

    m_tfunc_changed = true;
}

void SSAOStochasticTubeRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* line = kvs::LineObject::DownCast( object );
    BaseClass::attachObject( line );
    BaseClass::createRandomTexture();

    m_render_pass.create( BaseClass::shader(), false );

    // Create buffer object
    this->create_buffer_object( line );

    // Create transfer function texture
    this->create_transfer_function_texture();
}

void SSAOStochasticTubeRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    // Update buffer object
    this->update_buffer_object( kvs::LineObject::DownCast( object ) );

    // Update transfer function texture
    this->update_transfer_function_texture();
}

void SSAOStochasticTubeRenderer::Engine::setup(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( object );
    kvs::IgnoreUnusedVariable( camera );
    kvs::IgnoreUnusedVariable( light );

    // Setup transfer function texture
    if ( m_tfunc_changed ) { this->update_transfer_function_texture(); }

    // Setup shader program
    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    const auto M = kvs::OpenGL::ModelViewMatrix();
    const auto P = kvs::OpenGL::ProjectionMatrix();
    const auto N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    geom_pass.setUniform( "ModelViewMatrix", M );
    geom_pass.setUniform( "ProjectionMatrix", P );
    geom_pass.setUniform( "NormalMatrix", N );
    geom_pass.setUniform( "edge_factor", m_edge_factor );
}

void SSAOStochasticTubeRenderer::Engine::draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );

    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    this->draw_buffer_object( kvs::LineObject::DownCast( object ) );
}

void SSAOStochasticTubeRenderer::Engine::create_transfer_function_texture()
{
    const size_t width = m_tfunc.resolution();
    const auto table = m_tfunc.table();
    m_tfunc_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_tfunc_texture.setMagFilter( GL_LINEAR );
    m_tfunc_texture.setMinFilter( GL_LINEAR );
    m_tfunc_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT  );
    m_tfunc_texture.create( width, table.data() );
    m_tfunc_changed = false;

    kvs::Real32 min_value = 0.0f;
    kvs::Real32 max_value = 0.0f;
    if ( m_tfunc.hasRange() )
    {
        min_value = m_tfunc.minValue();
        max_value = m_tfunc.maxValue();
    }
    else
    {
        const auto* line = kvs::LineObject::DownCast( BaseClass::object() );
        const auto& values = line->sizes();
        min_value = values[0];
        max_value = values[1];
        for ( size_t i = 0; i < values.size(); i++ )
        {
            min_value = kvs::Math::Min( min_value, values[i] );
            max_value = kvs::Math::Max( max_value, values[i] );
        }
    }

    // Set min/max value to the geometry pass shader
    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    geom_pass.setUniform( "min_value", min_value );
    geom_pass.setUniform( "max_value", max_value );
}

void SSAOStochasticTubeRenderer::Engine::update_transfer_function_texture()
{
    m_tfunc_texture.release();
    this->create_transfer_function_texture();
}

void SSAOStochasticTubeRenderer::Engine::create_buffer_object( const kvs::LineObject* line )
{
    auto& geom_pass = m_render_pass.shaderProgram();

    const auto nvertices = line->numberOfVertices() * 2;
    const auto indices= BaseClass::randomIndices( nvertices );
    const auto indices_location = geom_pass.attributeLocation( "random_index" );
    m_buffer_object.manager().setVertexAttribArray( indices, indices_location, 2 );

    const auto values = ::QuadVertexValues( line );
    const auto values_location = geom_pass.attributeLocation( "value" );
    m_buffer_object.manager().setVertexAttribArray( values, values_location, 1 );

    const auto halo_size = m_render_pass.haloSize();
    const auto radius_size = m_render_pass.radiusSize();
    m_buffer_object.create( line, halo_size, radius_size );
}

void SSAOStochasticTubeRenderer::Engine::update_buffer_object( const kvs::LineObject* line )
{
    m_buffer_object.release();
    this->create_buffer_object( line );
}

void SSAOStochasticTubeRenderer::Engine::draw_buffer_object( const kvs::LineObject* line )
{
    // Random factors
    const size_t size = BaseClass::randomTextureSize();
    const int count = BaseClass::repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    // Update variables in geom pass shader
    auto& geom_pass = m_render_pass.shaderProgram();
    geom_pass.setUniform( "shape_texture", 0 );
    geom_pass.setUniform( "diffuse_texture", 1 );
    geom_pass.setUniform( "random_texture", 2 );
    geom_pass.setUniform( "transfer_function_texture", 3 );
    geom_pass.setUniform( "random_offset", random_offset );
    geom_pass.setUniform( "random_texture_size_inv", 1.0f / size );

    kvs::Texture::Binder unit2( BaseClass::randomTexture(), 2 );
    kvs::Texture::Binder unit3( m_tfunc_texture, 3 );
    m_buffer_object.draw( line );
}

} // end of namespace AmbientOcclusionRendering
