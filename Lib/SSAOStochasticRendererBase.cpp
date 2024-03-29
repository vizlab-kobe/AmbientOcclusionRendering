#include "SSAOStochasticRendererBase.h"
#include <kvs/OpenGL>


namespace AmbientOcclusionRendering
{

void SSAOStochasticRendererBase::exec(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    BaseClass::startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );

    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();
    if ( BaseClass::isWindowCreated() )
    {
        BaseClass::setWindowSize( width, height );
        BaseClass::setDevicePixelRatio( camera->devicePixelRatio() );
        BaseClass::setModelView( kvs::OpenGL::ModelViewMatrix() );
        BaseClass::setLightPosition( light->position() );

        const auto frame_width = BaseClass::framebufferWidth();
        const auto frame_height = BaseClass::framebufferHeight();
        BaseClass::createEnsembleBuffer( frame_width, frame_height );
        BaseClass::createEngine( object, camera, light );

        const auto enable_shading = kvs::RendererBase::isShadingEnabled();
        m_ao_buffer.createFramebuffer( frame_width, frame_height );
        m_ao_buffer.createShaderProgram( BaseClass::shader(), enable_shading );
    }

    if ( BaseClass::isWindowResized( width, height ) )
    {
        BaseClass::setWindowSize( width, height );

        // Update ensemble buffer
        BaseClass::ensembleBuffer().release();
        const auto frame_width = BaseClass::framebufferWidth();
        const auto frame_height = BaseClass::framebufferHeight();
        BaseClass::createEnsembleBuffer( frame_width, frame_height );

        // Update engine
        BaseClass::engine().update( object, camera, light );

        const auto enable_shading = kvs::RendererBase::isShadingEnabled();
        m_ao_buffer.updateFramebuffer( frame_width, frame_height );
        m_ao_buffer.updateShaderProgram( BaseClass::shader(), enable_shading );
    }

    if ( BaseClass::isObjectChanged( object ) )
    {
        // Clear ensemble buffer
        BaseClass::ensembleBuffer().clear();

        // Recreate engine
        BaseClass::engine().release();
        BaseClass::createEngine( object, camera, light );
    }

    BaseClass::setupEngine( object, camera, light );
    m_ao_buffer.setupShaderProgram( BaseClass::shader() );

    // Ensemble rendering.
    const auto m = kvs::OpenGL::ModelViewMatrix();
    const auto l = light->position();
    const size_t r = BaseClass::controllledRepetitions( m, l );
    for ( size_t i = 0; i < r; i++ )
    {
        // Render to the ensemble buffer.
        BaseClass::ensembleBuffer().bind();

        m_ao_buffer.bind();
        BaseClass::engine().draw( object, camera, light );
        BaseClass::engine().countRepetitions();
        m_ao_buffer.unbind();
        m_ao_buffer.draw();

        BaseClass::ensembleBuffer().unbind();

        // Progressive averaging.
        BaseClass::ensembleBuffer().add();
    }

    // Render to the framebuffer.
    BaseClass::ensembleBuffer().draw();

    kvs::OpenGL::Finish();
    BaseClass::stopTimer();
}

} // end of namespace AmbientOcclusionRendering
