/*****************************************************************************/
/**
 *  @file   StochasticRenderingCompositor.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOStochasticRenderingCompositor.h"


namespace
{

inline kvs::Vec2ui FrameBufferSize( const kvs::Camera* camera )
{
    const auto width = camera->windowWidth();
    const auto height = camera->windowHeight();
    const auto dpr = camera->devicePixelRatio();
    return kvs::Vec2ui( width * dpr, height * dpr );
}

} // end of namespace


namespace AmbientOcclusionRendering
{

void SSAOStochasticRenderingCompositor::onWindowCreated()
{
    const auto buf_size = ::FrameBufferSize( BaseClass::scene()->camera() );
    m_ao_buffer.createFramebuffer( buf_size[0], buf_size[1] );
    m_ao_buffer.createShaderProgram( this->shader(), true );
    BaseClass::onWindowCreated();
}

void SSAOStochasticRenderingCompositor::onWindowResized()
{
    const auto buf_size = ::FrameBufferSize( BaseClass::scene()->camera() );
    m_ao_buffer.createFramebuffer( buf_size[0], buf_size[1] );
    BaseClass::onWindowResized();
}

void SSAOStochasticRenderingCompositor::updateEngines()
{
    m_ao_buffer.updateShaderProgram( this->shader(), true );
    BaseClass::updateEngines();
}

void SSAOStochasticRenderingCompositor::setupEngines()
{
    m_ao_buffer.setupShaderProgram( this->shader() );
    BaseClass::setupEngines();
}

void SSAOStochasticRenderingCompositor::bindBuffer()
{
    BaseClass::bindBuffer();
    m_ao_buffer.bind();
}

void SSAOStochasticRenderingCompositor::unbindBuffer()
{
    m_ao_buffer.unbind();
    m_ao_buffer.draw();
    BaseClass::unbindBuffer();
}

} // end of namespace local
