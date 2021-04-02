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
inline int RandomNumber()
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
inline size_t NumberOfVertices( const kvs::PolygonObject* polygon )
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

} // end of namespace


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOStochasticPolygonRenderer class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::SSAOStochasticPolygonRenderer():
    SSAOStochasticRendererBase( new Engine() )
{
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
 *  @brief  Sets edge factor.
 *  @param  factor [in] edge factor
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::setEdgeFactor( const float factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( factor );
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::Engine::Engine()
{
    m_render_pass.setShaderFiles(
        "SSAO_SR_polygon_geom_pass.vert",
        "SSAO_SR_polygon_geom_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases buffer object and AO buffer resources.
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::release()
{
    m_buffer_object.release();
    m_render_pass.release();
}

/*===========================================================================*/
/**
 *  @brief  Create AO buffer resources.
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

    // Create render pass
    m_render_pass.create( BaseClass::shader(), false );

    // Create buffer object
    this->create_buffer_object( polygon );
}

/*===========================================================================*/
/**
 *  @brief  Update AO buffer resources.
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
    // Update buffer object
    this->update_buffer_object( kvs::PolygonObject::DownCast( object ) );
}

/*===========================================================================*/
/**
 *  @brief  Setups shader program.
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

    const auto M = kvs::OpenGL::ModelViewMatrix();
    const auto P = kvs::OpenGL::ProjectionMatrix();
    const auto N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );

    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    geom_pass.setUniform( "ModelViewMatrix", M );
    geom_pass.setUniform( "ModelViewProjectionMatrix", P * M );
    geom_pass.setUniform( "NormalMatrix", N );
    geom_pass.setUniform( "edge_factor", m_edge_factor );
}

/*===========================================================================*/
/**
 *  @brief  Draw object with AO.
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

    // Draw buffer object
    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    this->draw_buffer_object( kvs::PolygonObject::DownCast( object ) );
}

/*===========================================================================*/
/**
 *  @brief  Creates buffer object.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create_buffer_object(
    const kvs::PolygonObject* polygon )
{
    // Create buffer object
    const auto nvertices = ::NumberOfVertices( polygon );
    const auto indices = BaseClass::randomIndices( nvertices );
    auto location = m_render_pass.shaderProgram().attributeLocation( "random_index" );
    m_buffer_object.manager().setVertexAttribArray( indices, location, 2 );
    m_buffer_object.create( polygon );
}

/*===========================================================================*/
/**
 *  @brief  Updates buffer object.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::update_buffer_object(
    const kvs::PolygonObject* polygon )
{
    m_buffer_object.release();
    this->create_buffer_object( polygon );
}

/*===========================================================================*/
/**
 *  @brief  Draws buffer object.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::draw_buffer_object(
    const kvs::PolygonObject* polygon )
{
    // Random factors
    const size_t size = BaseClass::randomTextureSize();
    const int count = BaseClass::repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    // Update variables in geom pass shader
    auto& geom_pass = m_render_pass.shaderProgram();
    geom_pass.setUniform( "random_texture", 0 );
    geom_pass.setUniform( "random_offset", random_offset );
    geom_pass.setUniform( "random_texture_size_inv", 1.0f / size );

    // Draw buffer object
    kvs::Texture::Binder bind( BaseClass::randomTexture() );
    m_buffer_object.draw( polygon );
}

} // end of namespace AmbientOcclusionRendering
