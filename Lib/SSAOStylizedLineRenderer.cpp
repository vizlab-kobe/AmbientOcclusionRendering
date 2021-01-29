#include "SSAOStylizedLineRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/ShaderSource>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/String>


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
        this->createShaderProgram();
        this->createFramebuffer( framebuffer_width, framebuffer_height );
        BaseClass::createBufferObject( object );
    }

    if ( this->isWindowResized( width, height ) )
    {
        BaseClass::setWindowSize( width, height );
        this->updateFramebuffer( framebuffer_width, framebuffer_height );
    }

    if ( BaseClass::isObjectChanged( object ) )
    {
        this->updateShaderProgram();
        this->updateFramebuffer( framebuffer_width, framebuffer_height );
        BaseClass::updateBufferObject( object );
    }

    this->setupShaderProgram();

    // Ambient occlusion.
    m_ao_buffer.bind();
    BaseClass::drawBufferObject( camera );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();

    BaseClass::stopTimer();
}

void SSAOStylizedLineRenderer::createShaderProgram()
{
    m_ao_buffer.createShaderProgram( BaseClass::shadingModel(), BaseClass::isEnabledShading() );
}

void SSAOStylizedLineRenderer::updateShaderProgram()
{
    m_ao_buffer.updateShaderProgram( BaseClass::shadingModel(), BaseClass::isEnabledShading() );
}

void SSAOStylizedLineRenderer::setupShaderProgram()
{
    m_ao_buffer.setupShaderProgram( BaseClass::shadingModel() );

//    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 P = kvs::OpenGL::ProjectionMatrix();
//    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    m_ao_buffer.geometryPassShader().bind();
//    m_ao_buffer.geometryPassShader().setUniform( "ModelViewMatrix", M );
    m_ao_buffer.geometryPassShader().setUniform( "ProjectionMatrix", P );
//    m_ao_buffer.geometryPassShader().setUniform( "NormalMatrix", N );
    m_ao_buffer.geometryPassShader().setUniform( "shape_texture", 0 );
    m_ao_buffer.geometryPassShader().setUniform( "diffuse_texture", 1 );
    m_ao_buffer.geometryPassShader().unbind();
}

void SSAOStylizedLineRenderer::createFramebuffer( const size_t width, const size_t height )
{
    m_ao_buffer.createFramebuffer( width, height );
}

void SSAOStylizedLineRenderer::updateFramebuffer( const size_t width, const size_t height )
{
    m_ao_buffer.updateFramebuffer( width, height );
}

} // end of namespace AmbientOcclusionRendering
