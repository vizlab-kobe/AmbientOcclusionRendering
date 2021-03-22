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

/*===========================================================================*/
/**
 *  @brief  Sets a polygon offset.
 *  @param  offset [in] offset value
 */
/*===========================================================================*/
//void SSAOStochasticPolygonRenderer::setPolygonOffset( const float offset )
//{
//    static_cast<Engine&>( engine() ).setPolygonOffset( offset );
//}

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

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::Engine::Engine()
{
    m_geom_pass_vert_file = "EE_SSAO_SR_polygon_geom_pass.vert";
    m_geom_pass_frag_file = "EE_SSAO_SR_polygon_geom_pass.frag";
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::release()
{
    m_geom_pass_shader.release();
    m_buffer_object.release();
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
    BaseClass::setEnabledShading( has_normal );

    BaseClass::attachObject( object );
    BaseClass::createRandomTexture();

    this->create_geom_pass();
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

    this->setup_geom_pass();
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

    m_geom_pass_shader.bind();
    this->draw_buffer_object( kvs::PolygonObject::DownCast( object ) );
    m_geom_pass_shader.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Create buffer objects.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create_buffer_object( const kvs::PolygonObject* polygon )
{
    const auto nvertices = ::NumberOfVertices( polygon );
    const auto indices= BaseClass::randomIndices( nvertices );
    auto location = m_geom_pass_shader.attributeLocation( "random_index" );
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

    m_geom_pass_shader.setUniform( "random_texture", 0 );
    m_geom_pass_shader.setUniform( "random_offset", random_offset );
    m_geom_pass_shader.setUniform( "random_texture_size_inv", 1.0f / size );

    kvs::Texture::Binder bind3( randomTexture() );
    m_buffer_object.draw( polygon );
}

void SSAOStochasticPolygonRenderer::Engine::create_geom_pass()
{
    kvs::ShaderSource vert( m_geom_pass_vert_file );
    kvs::ShaderSource frag( m_geom_pass_frag_file );
    m_geom_pass_shader.build( vert, frag );
}

void SSAOStochasticPolygonRenderer::Engine::setup_geom_pass()
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 PM = kvs::OpenGL::ProjectionMatrix() * M;
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );

    m_geom_pass_shader.bind();
    m_geom_pass_shader.setUniform( "ModelViewMatrix", M );
    m_geom_pass_shader.setUniform( "ModelViewProjectionMatrix", PM );
    m_geom_pass_shader.setUniform( "NormalMatrix", N );
    m_geom_pass_shader.setUniform( "edge_factor", m_edge_factor );
    m_geom_pass_shader.unbind();
}

} // end of namespace local
