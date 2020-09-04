/*****************************************************************************/
/**
 *  @file   SSAOPolygonRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOPolygonRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOPolygonRenderer class.
 */
/*===========================================================================*/
SSAOPolygonRenderer::SSAOPolygonRenderer()
{
    m_drawable.setGeometryPassShaderFiles( "SSAO_geom_pass.vert", "SSAO_geom_pass.frag" );
    m_drawable.setOcclusionPassShaderFiles( "SSAO_occl_pass.vert", "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Executes rendering process.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOPolygonRenderer::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    BaseClass::startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );
    kvs::OpenGL::Enable( GL_DEPTH_TEST );

    auto* polygon = kvs::PolygonObject::DownCast( object );
    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( width * dpr );
    const size_t framebuffer_height = static_cast<size_t>( height * dpr );

    if ( BaseClass::isWindowCreated() )
    {
        BaseClass::setWindowSize( width, height );
        BaseClass::attachObject( object );
        BaseClass::createBufferObject( polygon );
        this->createShaderProgram();
        this->createFramebuffer( framebuffer_width, framebuffer_height );
    }

    if ( this->isWindowResized( width, height ) )
    {
        BaseClass::setWindowSize( width, height );
        this->updateFramebuffer( framebuffer_width, framebuffer_height );
    }

    if ( BaseClass::isObjectChanged( object ) )
    {
        BaseClass::attachObject( object );
        BaseClass::updateBufferObject( polygon );
        this->updateShaderProgram();
        this->updateFramebuffer( framebuffer_width, framebuffer_height );
    }

    // Ambient occlusion.
    m_drawable.bind();
    m_drawable.geometryPassShader().bind();
    BaseClass::drawBufferObject( polygon );
    m_drawable.geometryPassShader().unbind();
    m_drawable.unbind();
    m_drawable.draw();

    BaseClass::stopTimer();
}

void SSAOPolygonRenderer::createShaderProgram()
{
    m_drawable.createShaderProgram( BaseClass::shadingModel(), BaseClass::isEnabledShading() );
}

void SSAOPolygonRenderer::updateShaderProgram()
{
    m_drawable.updateShaderProgram( BaseClass::shadingModel(), BaseClass::isEnabledShading() );
}

void SSAOPolygonRenderer::createFramebuffer( const size_t width, const size_t height )
{
    m_drawable.createFramebuffer( width, height );
}

void SSAOPolygonRenderer::updateFramebuffer( const size_t width, const size_t height )
{
    m_drawable.updateFramebuffer( width, height );
}

} // end of namespace AmbientOcclusionRendering
