/*****************************************************************************/
/**
 *  @file   SSAOStylizedLineRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOStylizedLineRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/ShaderSource>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/String>
#include <kvs/IgnoreUnusedVariable>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOStylizedLineRenderer class.
 */
/*===========================================================================*/
SSAOStylizedLineRenderer::SSAOStylizedLineRenderer()
{
    m_ao_buffer.setGeometryPassShaderFiles(
        "SSAO_stylized_geom_pass.vert",
        "SSAO_stylized_geom_pass.frag" );

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
void SSAOStylizedLineRenderer::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( light );

    BaseClass::startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );
    kvs::OpenGL::Enable( GL_DEPTH_TEST );

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
void SSAOStylizedLineRenderer::create_shader_program()
{
    m_ao_buffer.createShaderProgram(
        BaseClass::shadingModel(),
        BaseClass::isEnabledShading() );
}

/*===========================================================================*/
/**
 *  @brief  Updates AO shader program.
 */
/*===========================================================================*/
void SSAOStylizedLineRenderer::update_shader_program()
{
    m_ao_buffer.updateShaderProgram(
        BaseClass::shadingModel(),
        BaseClass::isEnabledShading() );
}

/*===========================================================================*/
/**
 *  @brief  Setups AO shader program.
 */
/*===========================================================================*/
void SSAOStylizedLineRenderer::setup_shader_program()
{
    m_ao_buffer.setupShaderProgram( BaseClass::shadingModel() );

    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.bind();
    geom_pass.setUniform( "ProjectionMatrix", kvs::OpenGL::ProjectionMatrix() );
    geom_pass.setUniform( "shape_texture", 0 );
    geom_pass.setUniform( "diffuse_texture", 1 );
    geom_pass.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Creates AO framebuffer object
 *  @param  width [in] framebuffer width
 *  @param  height [in] framebuffer height
 */
/*===========================================================================*/
void SSAOStylizedLineRenderer::create_framebuffer(
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
void SSAOStylizedLineRenderer::update_framebuffer(
    const size_t width,
    const size_t height )
{
    m_ao_buffer.updateFramebuffer( width, height );
}

} // end of namespace AmbientOcclusionRendering
