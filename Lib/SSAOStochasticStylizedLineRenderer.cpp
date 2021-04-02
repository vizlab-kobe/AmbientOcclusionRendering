#include "SSAOStochasticStylizedLineRenderer.h"
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

} // end of namespace


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Creates a new SSAOStochasticStylizedLineRenderer class.
 */
/*===========================================================================*/
SSAOStochasticStylizedLineRenderer::SSAOStochasticStylizedLineRenderer():
    SSAOStochasticRendererBase( new Engine() )
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
 *  @brief  Creates a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticStylizedLineRenderer::Engine::Engine()
{
    m_render_pass.setShaderFiles(
        "SSAO_SR_stylized_geom_pass.vert",
        "SSAO_SR_stylized_geom_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases AO buffer and buffer object.
 */
/*===========================================================================*/
void SSAOStochasticStylizedLineRenderer::Engine::release()
{
    m_buffer_object.release();
    m_render_pass.release();
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

    m_render_pass.create( BaseClass::shader(), false );

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
    kvs::IgnoreUnusedVariable( object );
    kvs::IgnoreUnusedVariable( camera );
    kvs::IgnoreUnusedVariable( light );

    // Setup shader program
    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    const auto M = kvs::OpenGL::ModelViewMatrix();
    const auto P = kvs::OpenGL::ProjectionMatrix();
    const auto N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    geom_pass.setUniform( "ModelViewMatrix", M );
    geom_pass.setUniform( "ProjectionMatrix", P );
    geom_pass.setUniform( "NormalMatrix", N );
    geom_pass.setUniform( "opacity", m_line_opacity / 255.0f );
    geom_pass.setUniform( "edge_factor", m_edge_factor );
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
    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );

    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    this->draw_buffer_object( kvs::LineObject::DownCast( object ) );
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
    auto& geom_pass = m_render_pass.shaderProgram();

    // Create random index array
    const auto nvertices = line->numberOfVertices() * 2;
    const auto indices = BaseClass::randomIndices( nvertices );
    const auto indices_location = geom_pass.attributeLocation( "random_index" );
    m_buffer_object.manager().setVertexAttribArray( indices, indices_location, 2 );

    // Create buffer object
    const auto halo_size = m_render_pass.haloSize();
    const auto radius_size = m_render_pass.radiusSize();
    m_buffer_object.create( line, halo_size, radius_size );
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
    geom_pass.setUniform( "random_offset", random_offset );
    geom_pass.setUniform( "random_texture_size_inv", 1.0f / size );

    // Draw buffer object
    kvs::Texture::Binder unit( BaseClass::randomTexture(), 2 );
    m_buffer_object.draw( line );
}

} // end of namespace AmbientOcclusionRendering
