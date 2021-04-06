/*****************************************************************************/
/**
 *  @file   StochasticTetrahedraRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 * References:
 * [1] Naohisa Sakamoto, Koji Koyamada, "A Stochastic Approach for Rendering
 *     Multiple Irregular Volumes", In Proc. of IEEE Pacific Visualization
 *     2014 (VisNotes), pp.272-276, 2014.3.
 */
/*****************************************************************************/
#include "SSAOStochasticTetrahedraRenderer.h"
#include <cmath>
#include <kvs/OpenGL>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Assert>
#include <kvs/Message>
#include <kvs/Type>
#include <kvs/Xorshift128>
#include <kvs/TetrahedralCell>
#include <kvs/ProjectedTetrahedraTable>
#include <kvs/PreIntegrationTable2D>


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
 *  @brief  Constructs a new StochasticTetrahedraRenderer class.
 */
/*===========================================================================*/
SSAOStochasticTetrahedraRenderer::SSAOStochasticTetrahedraRenderer():
    SSAOStochasticRendererBase( new Engine() )
{
}

/*===========================================================================*/
/**
 *  @brief  Sets edge factor.
 *  @param  factor [in] edge factor
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setEdgeFactor( const float factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( factor );
}

/*===========================================================================*/
/**
 *  @brief  Sets a transfer function.
 *  @param  transfer_function [in] transfer function
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setTransferFunction(
    const kvs::TransferFunction& transfer_function )
{
    static_cast<Engine&>( engine() ).setTransferFunction( transfer_function );
}

/*===========================================================================*/
/**
 *  @brief  Sets a sampling step.
 *  @param  sampling_step [in] sampling step
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setSamplingStep( const float sampling_step )
{
    static_cast<Engine&>( engine() ).setSamplingStep( sampling_step );
}

/*===========================================================================*/
/**
 *  @brief  Returns transfer function.
 *  @return transfer function
 */
/*===========================================================================*/
const kvs::TransferFunction& SSAOStochasticTetrahedraRenderer::transferFunction() const
{
    return static_cast<const Engine&>( engine() ).transferFunction();
}

/*===========================================================================*/
/**
 *  @brief  Returns sampling step.
 *  @return sampling step
 */
/*===========================================================================*/
float SSAOStochasticTetrahedraRenderer::samplingStep() const
{
    return static_cast<const Engine&>( engine() ).samplingStep();
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticTetrahedraRenderer::Engine::Engine()
{
    m_render_pass.setShaderFiles(
        "SSAO_SR_tetrahedra_geom_pass.vert",
        "SSAO_SR_tetrahedra_geom_pass.geom",
        "SSAO_SR_tetrahedra_geom_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::release()
{
    m_buffer_object.release();
    m_render_pass.release();
    m_transfer_function_buffer.release();
    m_preintegration_buffer.release();
    m_decomposition_buffer.release();
    m_transfer_function_changed = true;
}

/*===========================================================================*/
/**
 *  @brief  Create shaders and buffer objects.
 *  @param  object [in] pointer to the polygon object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* volume = kvs::UnstructuredVolumeObject::DownCast( object );
    BaseClass::attachObject( object );
    BaseClass::createRandomTexture();

    // Create shader program
    {
        m_render_pass.create(
            BaseClass::shader(),
            BaseClass::isShadingEnabled(),
            BaseClass::depthTexture().isCreated() );

        auto& shader_program = m_render_pass.shaderProgram();
        kvs::ProgramObject::Binder bind( shader_program );
        shader_program.setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
        shader_program.setUniform( "random_texture", 0 );
        shader_program.setUniform( "preintegration_texture", 1 );
        shader_program.setUniform( "decomposition_texture", 2 );
        shader_program.setUniform( "transfer_function_texture", 3 );
        shader_program.setUniform( "T_texture", 4 );
        shader_program.setUniform( "invT_texture", 5 );
    }

    this->create_buffer_object( volume );
    this->create_transfer_function_texture();
    this->create_decomposition_texture();
}

/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  polygon [in] pointer to the polygon object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    this->update_buffer_object( kvs::UnstructuredVolumeObject::DownCast( object ) );
    this->update_transfer_function_texture();
}

/*===========================================================================*/
/**
 *  @brief  Set up.
 *  @param  polygon [in] pointer to the polygon object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::setup(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( object );
    kvs::IgnoreUnusedVariable( camera );
    kvs::IgnoreUnusedVariable( light );

    if ( m_transfer_function_changed )
    {
        this->update_transfer_function_texture();
    }

    m_render_pass.setup( BaseClass::shader() );

    auto& shader_program = m_render_pass.shaderProgram();
    shader_program.bind();
    shader_program.setUniform( "maxT", m_preintegration_buffer.Tmax() );
    shader_program.setUniform( "delta", 0.5f / m_transfer_function.resolution() );
    shader_program.setUniform( "edge_factor", m_edge_factor );
    shader_program.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Draw an ensemble.
 *  @param  object [in] pointer to the unstructured volume object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    this->draw_buffer_object( kvs::UnstructuredVolumeObject::DownCast( object ) );
}

/*===========================================================================*/
/**
 *  @brief  Creates pre-integration texture.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_transfer_function_texture()
{
    m_preintegration_buffer.create( m_transfer_function );
    m_transfer_function_buffer.create( m_transfer_function );
    m_transfer_function_changed = false;

    const auto inv_size = m_preintegration_buffer.inverseTextureSize();
    auto& geom_pass = m_render_pass.shaderProgram();
    geom_pass.bind();
    geom_pass.setUniform( "delta2", 0.5f / inv_size );
    geom_pass.unbind();
}

void SSAOStochasticTetrahedraRenderer::Engine::update_transfer_function_texture()
{
    m_transfer_function_buffer.release();
    m_preintegration_buffer.release();
    this->create_transfer_function_texture();
}

/*===========================================================================*/
/**
 *  @brief  Creates decomposition texture.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_decomposition_texture()
{
    m_decomposition_buffer.create();
}

/*===========================================================================*/
/**
 *  @brief  Creates buffer object.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_buffer_object(
    const kvs::UnstructuredVolumeObject* volume )
{
    m_buffer_object.create( volume, m_render_pass.shaderProgram() );
}

/*===========================================================================*/
/**
 *  @brief  Updates buffer object.
 *  @param  volume [in] pointer to the unstructured volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::update_buffer_object(
    const kvs::UnstructuredVolumeObject* volume )
{
    m_buffer_object.release();
    this->create_buffer_object( volume );
}

/*===========================================================================*/
/**
 *  @brief  Draws buffer object.
 *  @param  volume [in] pointer to the unstructured volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::draw_buffer_object(
    const kvs::UnstructuredVolumeObject* volume )
{
    kvs::OpenGL::WithEnabled d( GL_DEPTH_TEST );

    // Random factors
    const size_t size = BaseClass::randomTextureSize();
    const int count = BaseClass::repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    auto& geom_pass = m_render_pass.shaderProgram();
    kvs::ProgramObject::Binder bind( geom_pass );
    geom_pass.setUniform( "random_offset", random_offset );

    // Draw buffer object
    kvs::Texture::Binder unit0( randomTexture(), 0 );
    kvs::Texture::Binder unit1( m_preintegration_buffer.texture(), 1 );
    kvs::Texture::Binder unit2( m_decomposition_buffer.texture(), 2 );
    kvs::Texture::Binder unit3( m_transfer_function_buffer.texture(), 3 );
    kvs::Texture::Binder unit4( m_preintegration_buffer.T(), 4 );
    kvs::Texture::Binder unit5( m_preintegration_buffer.Tinverse(), 5 );
    m_buffer_object.draw( volume );
}

} // end of namespace AmbientOcclusionRendering
