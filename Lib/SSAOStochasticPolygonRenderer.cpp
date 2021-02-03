#include "SSAOStochasticPolygonRenderer.h"
#include <cmath>
#include <kvs/OpenGL>
#include <kvs/PolygonObject>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Assert>
#include <kvs/Message>
#include <kvs/Xorshift128>


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
    StochasticRendererBase( new Engine() )
{
}

/*===========================================================================*/
/**
 *  @brief  Sets a polygon offset.
 *  @param  offset [in] offset value
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::setPolygonOffset( const float offset )
{
    static_cast<Engine&>( engine() ).setPolygonOffset( offset );
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
 *  @brief  Sets radius of sampling sphere.
 *  @param  radius [in] radius of sampling sphere
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

/*===========================================================================*/
/**
 *  @brief  Sets number of sampling points
 *  @param  nsamples [in] number of sampling points
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

/*===========================================================================*/
/**
 *  @brief  Returns radius of sampling sphere.
 *  @return radius of sampling sphere
 */
/*===========================================================================*/
kvs::Real32 SSAOStochasticPolygonRenderer::samplingSphereRadius() const
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

/*===========================================================================*/
/**
 *  @brief  Returns number of sampling points
 *  @return number of sampling points
 */
/*===========================================================================*/
size_t SSAOStochasticPolygonRenderer::numberOfSamplingPoints() const
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::Engine::Engine():
    m_edge_factor( 0.0f ),
    m_polygon_offset( 0.0f )
{
    m_ao_buffer.setGeometryPassShaderFiles(
        "SSAO_SR_polygon_geom_pass.vert",
        "SSAO_SR_polygon_geom_pass.frag" );

    m_ao_buffer.setOcclusionPassShaderFiles(
        "SSAO_occl_pass.vert",
        "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases buffer object and AO buffer resources.
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::release()
{
    m_ao_buffer.release();
    m_buffer_object.release();
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

    // Create shader program
    m_ao_buffer.createShaderProgram( BaseClass::shader(), BaseClass::isShadingEnabled() );

    // Create framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    m_ao_buffer.createFramebuffer( framebuffer_width, framebuffer_height );

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
    // Update shader program
    m_ao_buffer.updateShaderProgram( BaseClass::shader(), BaseClass::isShadingEnabled() );

    // Update framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    m_ao_buffer.updateFramebuffer( framebuffer_width, framebuffer_height );

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
    // Setup shader program
    m_ao_buffer.setupShaderProgram( this->shader() );

    // Setup additional variables in geom pass shader
    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.bind();
    geom_pass.setUniform( "polygon_offset", m_polygon_offset );
    geom_pass.setUniform( "edge_factor", m_edge_factor );
    geom_pass.unbind();
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
    m_ao_buffer.bind();
    this->draw_buffer_object( kvs::PolygonObject::DownCast( object ) );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();
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
    // Create random index array
    const size_t nvertices = ::NumberOfVertices( polygon );
    const auto tex_size = randomTextureSize();
    kvs::ValueArray<kvs::UInt16> indices( nvertices * 2 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        const unsigned int count = i * 12347;
        indices[ 2 * i + 0 ] = static_cast<kvs::UInt16>( ( count ) % tex_size );
        indices[ 2 * i + 1 ] = static_cast<kvs::UInt16>( ( count / tex_size ) % tex_size );
    }

    // Create buffer object
    auto location = m_ao_buffer.geometryPassShader().attributeLocation( "random_index" );
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
    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.setUniform( "random_texture", 0 );
    geom_pass.setUniform( "random_offset", random_offset );
    geom_pass.setUniform( "random_texture_size_inv", 1.0f / size );

    // Draw buffer object
    kvs::Texture::Binder bind( BaseClass::randomTexture() );
    m_buffer_object.draw( polygon );
}

} // end of namespace AmbientOcclusionRendering
