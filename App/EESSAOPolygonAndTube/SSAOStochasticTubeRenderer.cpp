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

namespace local
{

SSAOStochasticTubeRenderer::SSAOStochasticTubeRenderer():
    StochasticRendererBase( new Engine() )
{
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

void SSAOStochasticTubeRenderer::setEdgeFactor( const float edge_factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( edge_factor );
}

SSAOStochasticTubeRenderer::Engine::RenderPass::RenderPass(
    Engine::BufferObject& buffer_object,
    RenderPass::Parent* parent ):
    BaseRenderPass( buffer_object ),
    m_parent( parent )
{
    this->setShaderFiles(
        "EE_SSAO_SR_tube_geom_pass.vert",
        "EE_SSAO_SR_tube_geom_pass.frag" );
}

void SSAOStochasticTubeRenderer::Engine::RenderPass::create(
    const kvs::Shader::ShadingModel& model,
    const bool enable )
{
    kvs::IgnoreUnusedVariable( enable );

    auto& shader_program = this->shaderProgram();
    kvs::ShaderSource vert( this->vertexShaderFile() );
    kvs::ShaderSource frag( this->fragmentShaderFile() );
    shader_program.build( vert, frag );
}

void SSAOStochasticTubeRenderer::Engine::RenderPass::setup(
    const kvs::Shader::ShadingModel& model )
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 P = kvs::OpenGL::ProjectionMatrix();
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );

    auto& shader_program = this->shaderProgram();
    kvs::ProgramObject::Binder bind( shader_program );
    shader_program.setUniform( "ModelViewMatrix", M );
    shader_program.setUniform( "ProjectionMatrix", P );
    shader_program.setUniform( "NormalMatrix", N );
}

void SSAOStochasticTubeRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* line = kvs::LineObject::DownCast( object );
    BaseClass::attachObject( line );
    BaseClass::createRandomTexture();

    m_render_pass.create( BaseClass::shader(), BaseClass::isShadingEnabled() );

    this->create_buffer_object( line );
    this->create_transfer_function_texture();
}

void SSAOStochasticTubeRenderer::Engine::update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    this->update_buffer_object( kvs::LineObject::DownCast( object ) );
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

    if ( m_tfunc_changed ) { this->update_transfer_function_texture(); }

    m_render_pass.setup( BaseClass::shader() );

    auto& shader_program = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( shader_program );
    shader_program.setUniform( "edge_factor", m_edge_factor );
}

void SSAOStochasticTubeRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::SetPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    auto& shader_program = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( shader_program );
    this->draw_buffer_object( kvs::LineObject::DownCast( object ) );
}

void SSAOStochasticTubeRenderer::Engine::create_transfer_function_texture()
{
    const size_t width = m_tfunc.resolution();
    const kvs::ValueArray<kvs::Real32> table = m_tfunc.table();
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

    auto& shader = m_render_pass.shaderProgram();
    shader.bind();
    shader.setUniform( "min_value", min_value );
    shader.setUniform( "max_value", max_value );
    shader.unbind();
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
    auto indices_location = geom_pass.attributeLocation( "random_index" );
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
    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );

    const size_t size = BaseClass::randomTextureSize();
    const int count = BaseClass::repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    auto& shader = m_render_pass.shaderProgram();
    shader.setUniform( "shape_texture", 0 );
    shader.setUniform( "diffuse_texture", 1 );
    shader.setUniform( "random_texture", 2 );
    shader.setUniform( "transfer_function_texture", 3 );
    shader.setUniform( "random_offset", random_offset );
    shader.setUniform( "random_texture_size_inv", 1.0f / size );

    kvs::Texture::Binder unit2( BaseClass::randomTexture(), 2 );
    kvs::Texture::Binder unit3( m_tfunc_texture, 3 );
    m_buffer_object.draw( line );
}

} // end of namespace local
