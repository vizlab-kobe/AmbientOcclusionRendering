/*****************************************************************************/
/**
 *  @file   SSAOPolygonRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOPolygonRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/IgnoreUnusedVariable>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOPolygonRenderer class.
 */
/*===========================================================================*/
SSAOPolygonRenderer::SSAOPolygonRenderer()
{
    BaseClass::renderPass().setShaderFiles(
        "SSAO_geom_pass.vert",
        "SSAO_geom_pass.frag" );

    m_ao_buffer.setOcclusionPassShaderFiles(
        "SSAO_occl_pass.vert",
        "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Executes rendering process.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOPolygonRenderer::exec(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( light );

    BaseClass::startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );

    auto* polygon = kvs::PolygonObject::DownCast( object );
    const bool has_normal = polygon->normals().size() > 0;
    BaseClass::setShadingEnabled( has_normal );

    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( width * dpr );
    const size_t framebuffer_height = static_cast<size_t>( height * dpr );

    if ( BaseClass::isWindowCreated() )
    {
        BaseClass::setWindowSize( width, height );
        this->create_shader_program();
        this->create_framebuffer( framebuffer_width, framebuffer_height );
        BaseClass::createBufferObject( object );
    }

    if ( this->isWindowResized( width, height ) )
    {
        BaseClass::setWindowSize( width, height );
        this->update_framebuffer( framebuffer_width, framebuffer_height );
    }

    if ( BaseClass::isObjectChanged( object ) )
    {
        this->update_shader_program();
        this->update_framebuffer( framebuffer_width, framebuffer_height );
        BaseClass::updateBufferObject( object );
    }

    this->setup_shader_program();

    m_ao_buffer.bind();
    BaseClass::drawBufferObject( camera );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();

    BaseClass::stopTimer();
}

/*===========================================================================*/
/**
 *  @brief  Creates AO shader program.
 */
/*===========================================================================*/
void SSAOPolygonRenderer::create_shader_program()
{
    const auto enabled = BaseClass::isShadingEnabled();
    auto& model = BaseClass::shadingModel();
    model.two_side_lighting = BaseClass::isTwoSideLightingEnabled();
    BaseClass::renderPass().create( model, false );
    m_ao_buffer.createShaderProgram( model, enabled );
}

/*===========================================================================*/
/**
 *  @brief  Updates AO shader program.
 */
/*===========================================================================*/
void SSAOPolygonRenderer::update_shader_program()
{
    const auto enabled = BaseClass::isShadingEnabled();
    auto& model = BaseClass::shadingModel();
    model.two_side_lighting = BaseClass::isTwoSideLightingEnabled();
    BaseClass::renderPass().create( model, false );
    m_ao_buffer.updateShaderProgram( model, enabled );
}

/*===========================================================================*/
/**
 *  @brief  Setups AO shader program.
 */
/*===========================================================================*/
void SSAOPolygonRenderer::setup_shader_program()
{
    const auto M = kvs::OpenGL::ModelViewMatrix();
    const auto P = kvs::OpenGL::ProjectionMatrix();
    const auto N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    auto& geom_pass = BaseClass::renderPass().shaderProgram();
    geom_pass.bind();
    geom_pass.setUniform( "ModelViewMatrix", M );
    geom_pass.setUniform( "ModelViewProjectionMatrix", P * M );
    geom_pass.setUniform( "NormalMatrix", N );
    geom_pass.unbind();

    m_ao_buffer.setupShaderProgram( BaseClass::shadingModel() );
}

/*===========================================================================*/
/**
 *  @brief  Creates AO framebuffer object
 *  @param  width [in] framebuffer width
 *  @param  height [in] framebuffer height
 */
/*===========================================================================*/
void SSAOPolygonRenderer::create_framebuffer(
    const size_t width,
    const size_t height )
{
    m_ao_buffer.createFramebuffer( width, height );
}

/*===========================================================================*/
/**
 *  @brief  Updates AO framebuffer object
 *  @param  width [in] framebuffer width
 *  @param  height [in] framebuffer height
 */
/*===========================================================================*/
void SSAOPolygonRenderer::update_framebuffer(
    const size_t width,
    const size_t height )
{
    m_ao_buffer.updateFramebuffer( width, height );
}

} // end of namespace AmbientOcclusionRendering
