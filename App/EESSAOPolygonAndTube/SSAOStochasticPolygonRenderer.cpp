#include "SSAOStochasticPolygonRenderer.h"
#include <cmath>
#include <kvs/OpenGL>
#include <kvs/PolygonObject>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Assert>
#include <kvs/Message>
#include <kvs/Xorshift128>
#include <kvs/IgnoreUnusedVariable>


namespace
{

/*===========================================================================*/
/**
 *  @brief  Returns a random number as integer value.
 *  @return random number
 */
/*===========================================================================*/
int RandomNumber()
{
    const int C = 12347;
    static kvs::Xorshift128 R;
    return C * R.randInteger();
}

/*===========================================================================*/
/**
 *  @brief  Returns number of vertices of the polygon object
 *  @param  polygon [in] pointer to the polygon object
 *  @return number of vertices
 */
/*===========================================================================*/
size_t NumberOfVertices( const kvs::PolygonObject* polygon )
{
    if ( polygon->connections().size() > 0 &&
         ( polygon->normalType() == kvs::PolygonObject::PolygonNormal ||
           polygon->colorType() == kvs::PolygonObject::PolygonColor ) )
    {
        const size_t nfaces = polygon->numberOfConnections();
        return nfaces * 3;
    }

    return polygon->numberOfVertices();
}

}


namespace local
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOStochasticPolygonRenderer class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::SSAOStochasticPolygonRenderer():
    StochasticRendererBase( new Engine() )
{
}

void SSAOStochasticPolygonRenderer::setEdgeFactor( const float edge_factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( edge_factor );
}

void SSAOStochasticPolygonRenderer::setDepthOffset( const kvs::Vec2& offset )
{
    static_cast<Engine&>( engine() ).setDepthOffset( offset );
}

void SSAOStochasticPolygonRenderer::setDepthOffset( const float factor, const float units )
{
    static_cast<Engine&>( engine() ).setDepthOffset( factor, units );
}

SSAOStochasticPolygonRenderer::Engine::RenderPass::RenderPass(
    Engine::BufferObject& buffer_object,
    RenderPass::Parent* parent ):
    BaseRenderPass( buffer_object ),
    m_parent( parent )
{
    this->setShaderFiles(
        "EE_SSAO_SR_polygon_geom_pass.vert",
        "EE_SSAO_SR_polygon_geom_pass.frag" );
}

void SSAOStochasticPolygonRenderer::Engine::RenderPass::create(
    const kvs::Shader::ShadingModel& model,
    const bool enable )
{
    kvs::IgnoreUnusedVariable( enable );

    auto& shader_program = this->shaderProgram();
    kvs::ShaderSource vert( this->vertexShaderFile() );
    kvs::ShaderSource frag( this->fragmentShaderFile() );
    shader_program.build( vert, frag );
}

void SSAOStochasticPolygonRenderer::Engine::RenderPass::setup(
    const kvs::Shader::ShadingModel& model )
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 PM = kvs::OpenGL::ProjectionMatrix() * M;
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );

    auto& shader_program = this->shaderProgram();
    kvs::ProgramObject::Binder bind( shader_program );
    shader_program.setUniform( "ModelViewMatrix", M );
    shader_program.setUniform( "ModelViewProjectionMatrix", PM );
    shader_program.setUniform( "NormalMatrix", N );
}

/*===========================================================================*/
/**
 *  @brief  Create shaders, VBO, and framebuffers.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* polygon = kvs::PolygonObject::DownCast( object );
    const bool has_normal = polygon->normals().size() > 0;
    BaseClass::setShadingEnabled( has_normal );

    BaseClass::attachObject( object );
    BaseClass::createRandomTexture();

    m_render_pass.create( BaseClass::shader(), BaseClass::isShadingEnabled() );

    this->create_buffer_object( polygon );
}

/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
}

/*===========================================================================*/
/**
 *  @brief  Set up.
 *  @param  polygon [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::setup(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( object );
    kvs::IgnoreUnusedVariable( camera );
    kvs::IgnoreUnusedVariable( light );

    m_render_pass.setup( BaseClass::shader() );

    auto& shader_program = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( shader_program );
    shader_program.setUniform( "edge_factor", m_edge_factor );
}

/*===========================================================================*/
/**
 *  @brief  Draw an ensemble.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    // Depth offset
    if ( !kvs::Math::IsZero( m_depth_offset[0] ) )
    {
        kvs::OpenGL::SetPolygonOffset( m_depth_offset[0], m_depth_offset[1] );
        kvs::OpenGL::Enable( GL_POLYGON_OFFSET_FILL );
    }

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::SetPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    auto& shader_program = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( shader_program );
    this->draw_buffer_object( kvs::PolygonObject::DownCast( object ) );
}

/*===========================================================================*/
/**
 *  @brief  Create buffer objects.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create_buffer_object( const kvs::PolygonObject* polygon )
{
    auto& shader_program = m_render_pass.shaderProgram();
    const auto nvertices = ::NumberOfVertices( polygon );
    const auto indices= BaseClass::randomIndices( nvertices );
    auto location = shader_program.attributeLocation( "random_index" );
    m_buffer_object.manager().setVertexAttribArray( indices, location, 2 );
    m_buffer_object.create( polygon );
}

void SSAOStochasticPolygonRenderer::Engine::draw_buffer_object( const kvs::PolygonObject* polygon )
{
    const size_t size = randomTextureSize();
    const int count = repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    auto& shader_program = m_render_pass.shaderProgram();
    shader_program.setUniform( "random_texture", 0 );
    shader_program.setUniform( "random_offset", random_offset );
    shader_program.setUniform( "random_texture_size_inv", 1.0f / size );

    kvs::Texture::Binder bind3( randomTexture() );
    m_buffer_object.draw( polygon );
}

} // end of namespace local
