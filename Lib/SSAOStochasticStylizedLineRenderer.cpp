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

} // end of namespace


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Creates a new SSAOStochasticStylizedLineRenderer class.
 */
/*===========================================================================*/
SSAOStochasticStylizedLineRenderer::SSAOStochasticStylizedLineRenderer():
    StochasticRendererBase( new Engine() )
{
}

/*===========================================================================*/
/**
 *  @brief  Sets edge factor.
 *  @param  factor [in] edge factor
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::setEdgeFactor( const float factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( factor );
}

/*===========================================================================*/
/**
 *  @brief  Sets line opacity.
 *  @param  opacity [in] line opacity
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::setOpacity( const kvs::UInt8 opacity )
{
    static_cast<Engine&>( engine() ).setOpacity( opacity );
}

/*===========================================================================*/
/**
 *  @brief  Sets radius size of tubeline.
 *  @param  size [in] radius size
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::setRadiusSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setRadiusSize( size );
}

/*===========================================================================*/
/**
 *  @brief  Sets halo size of tubeline.
 *  @param  size [in] halo size
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::setHaloSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setHaloSize( size );
}

/*===========================================================================*/
/**
 *  @brief  Sets radius of sampling sphere.
 *  @param  radius [in] radius of sampling sphere
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

/*===========================================================================*/
/**
 *  @brief  Sets number of sampling points.
 *  @param  nsamples [in] number of sampling points
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

/*===========================================================================*/
/**
 *  @brief  Returns line opacity.
 *  @return line opacity
 */
/*===========================================================================*/
kvs::UInt8 SSAOStochasticStylizedLineRenderer::opacity() const
{
    return static_cast<const Engine&>( engine() ).opacity();
}

/*===========================================================================*/
/**
 *  @brief  Returns radius size of tubeline.
 *  @return radius size
 */
/*===========================================================================*/
kvs::Real32 SSAOStochasticStylizedLineRenderer::radiusSize() const
{
    return static_cast<const Engine&>( engine() ).radiusSize();
}

/*===========================================================================*/
/**
 *  @brief  Returns halo size of tubeline.
 *  @return halo size
 */
/*===========================================================================*/
kvs::Real32 SSAOStochasticStylizedLineRenderer::haloSize() const
{
    return static_cast<const Engine&>( engine() ).haloSize();
}

/*===========================================================================*/
/**
 *  @brief  Returns radius of sampling sphere.
 *  @return radius of sampling sphere
 */
/*===========================================================================*/
kvs::Real32 SSAOStochasticStylizedLineRenderer::samplingSphereRadius()
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

/*===========================================================================*/
/**
 *  @brief  Returns number of sampling points.
 *  @return number of sampling points
 */
/*===========================================================================*/
size_t SSAOStochasticStylizedLineRenderer::numberOfSamplingPoints()
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

/*===========================================================================*/
/**
 *  @brief  Creates a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticStylizedLineRenderer::Engine::Engine():
    m_edge_factor( 0.0f ),
    m_line_opacity( 255 ),
    m_radius_size( 0.05f ),
    m_halo_size( 0.0f )
{
    m_ao_buffer.setGeometryPassShaderFiles(
        "SSAO_SR_stylized_geom_pass.vert",
        "SSAO_SR_stylized_geom_pass.frag" );

    m_ao_buffer.setOcclusionPassShaderFiles(
        "SSAO_occl_pass.vert",
        "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases AO buffer and buffer object.
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::release()
{
    m_buffer_object.release();
    m_ao_buffer.release();
}

/*===========================================================================*/
/**
 *  @brief  Creates AO buffer and buffer object.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* line = kvs::LineObject::DownCast( object );
    BaseClass::attachObject( line );
    BaseClass::createRandomTexture();

    // Create shader program
    m_ao_buffer.createShaderProgram( BaseClass::shader(), BaseClass::isShadingEnabled() );

    // Create framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    m_ao_buffer.createFramebuffer( framebuffer_width, framebuffer_height );

    // Create buffer object
    this->create_buffer_object( line );
}

/*===========================================================================*/
/**
 *  @brief  Updates AO buffer and buffer object.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::update(
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
    this->update_buffer_object( kvs::LineObject::DownCast( object ) );
}

/*===========================================================================*/
/**
 *  @brief  Setups AO buffer and buffer object.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::setup(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    // Setup shader program
    m_ao_buffer.setupShaderProgram( this->shader() );

    // Setup additional variables in geom pass shader
    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.bind();
    geom_pass.setUniform( "ProjectionMatrix", kvs::OpenGL::ProjectionMatrix() );
    geom_pass.setUniform( "opacity", m_line_opacity / 255.0f );
    geom_pass.setUniform( "edge_factor", m_edge_factor );
    geom_pass.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Draws buffer object with AO buffer.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    m_ao_buffer.bind();
    this->draw_buffer_object( kvs::LineObject::DownCast( object ) );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();
}

/*===========================================================================*/
/**
 *  @brief  Creaets buffer object.
 *  @param  line [in] pointer to the line object
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::create_buffer_object(
    const kvs::LineObject* line )
{
    // Create random index array
    const size_t nvertices = line->numberOfVertices() * 2;
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
    m_buffer_object.create( line, m_halo_size, m_radius_size );
}

/*===========================================================================*/
/**
 *  @brief  Updates buffer object.
 *  @param  line [in] pointer to the line object
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::update_buffer_object(
    const kvs::LineObject* line )
{
    m_buffer_object.release();
    this->create_buffer_object( line );
}

/*===========================================================================*/
/**
 *  @brief  Draws buffer object.
 *  @param  line [in] pointer to the line object
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::draw_buffer_object(
    const kvs::LineObject* line )
{
    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );

    // Random factors
    const size_t size = BaseClass::randomTextureSize();
    const int count = BaseClass::repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    // Update variables in geom pass shader
    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.setUniform( "shape_texture", 0 );
    geom_pass.setUniform( "diffuse_texture", 1 );
    geom_pass.setUniform( "random_texture", 2 );
    geom_pass.setUniform( "random_offset", random_offset );
    geom_pass.setUniform( "random_texture_size_inv", 1.0f / size );

    // Draw buffer object
    kvs::Texture::Binder unit( BaseClass::randomTexture(), 2 );
    m_buffer_object.draw( line );
}

} // end of namespace AmbientOcclusionRendering
