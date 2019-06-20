/*****************************************************************************/
/**
 *  @file   StochasticRendererBase.cpp
 *  @author Jun Nishimura, Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright (c) Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id$
 */
/*****************************************************************************/
#include "SSAOStochasticRendererBase.h"
#include <kvs/ObjectBase>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/OpenGL>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new StochasticRendererBase class.
 *  @param  engine [in] pointer to the stochastic rendering engine
 */
/*===========================================================================*/
SSAOStochasticRendererBase::SSAOStochasticRendererBase( AmbientOcclusionRendering::SSAOStochasticRenderingEngine* engine ):
    m_width( 0 ),
    m_height( 0 ),
    m_repetition_level( 1 ),
    m_coarse_level( 1 ),
    m_enable_lod( false ),
    m_enable_refinement( false ),
    m_shader( new kvs::Shader::Lambert() ),
    m_engine( engine )
{
}

/*===========================================================================*/
/**
 *  @brief  Destroys the StochasticRendererBase class.
 */
/*===========================================================================*/
SSAOStochasticRendererBase::~SSAOStochasticRendererBase()
{
    if ( m_shader ) delete m_shader;
    if ( m_engine ) delete m_engine;
}

/*===========================================================================*/
/**
 *  @brief  Releases object resouces from GPU.
 */
/*===========================================================================*/
void SSAOStochasticRendererBase::release()
{
    KVS_ASSERT( m_engine );

    if ( m_engine->object() )
    {
        m_engine->detachObject();
        m_engine->release();
    }
}

/*===========================================================================*/
/**
 *  @brief  Executes the rendering process.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camra
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticRendererBase::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );

    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();
    const bool window_created = m_width == 0 && m_height == 0;
    if ( window_created )
    {
        m_width = width;
        m_height = height;
        m_ensemble_buffer.create( width, height );
        m_ensemble_buffer.clear();
        m_ssao_framebuffer.create( width, height );
        m_ssao_framebuffer.clear();
        m_modelview = kvs::OpenGL::ModelViewMatrix();
        m_light_position = light->position();
        m_engine->setShader( &shader() );
        m_engine->setRepetitionLevel( m_repetition_level );
        m_engine->setEnabledShading( kvs::RendererBase::isEnabledShading() );
        m_engine->create( object, camera, light );
    }

    const bool window_resized = m_width != width || m_height != height;
    if ( window_resized )
    {
        m_width = width;
        m_height = height;
        m_ensemble_buffer.release();
        m_ssao_framebuffer.release();
        m_ensemble_buffer.create( width, height );
        m_ensemble_buffer.clear();
        m_ssao_framebuffer.create( width, height );
        m_ssao_framebuffer.clear();
        m_engine->update( object, camera, light );
    }

    const bool object_changed = m_engine->object() != object;
    if ( object_changed )
    {
        m_ensemble_buffer.clear();
        m_ssao_framebuffer.clear();
        m_engine->release();
        m_engine->setShader( &shader() );
        m_engine->setEnabledShading( kvs::RendererBase::isEnabledShading() );
        m_engine->create( object, camera, light );
    }

    // LOD control.
    size_t repetitions = m_repetition_level;
    kvs::Vec3 light_position = light->position();
    kvs::Mat4 modelview = kvs::OpenGL::ModelViewMatrix();
    if ( m_light_position != light_position || m_modelview != modelview )
    {
        if ( m_enable_lod )
        {
            repetitions = m_coarse_level;
        }
        m_light_position = light_position;
        m_modelview = modelview;
        m_ensemble_buffer.clear();
        m_ssao_framebuffer.clear();
    }

    // Setup engine.
    const bool reset_count = !m_enable_refinement;
    if ( reset_count ) m_engine->resetRepetitions();
    m_engine->setup( object, camera, light );

    // Ensemble rendering.
    if ( reset_count ) m_ensemble_buffer.clear();
    for ( size_t i = 0; i < repetitions; i++ )
    {
        // Render to the ensemble buffer.
        m_ssao_framebuffer.bind();
        m_engine->draw( object, camera, light );
        m_ssao_framebuffer.unbind();
        m_ensemble_buffer.bind();
        m_ssao_framebuffer.draw();
        m_engine->countRepetitions();
        m_ensemble_buffer.unbind();

        // Progressive averaging.
        m_ensemble_buffer.add();
    }

    // Render to the framebuffer.
    m_ensemble_buffer.draw();

    kvs::OpenGL::Finish();
    stopTimer();
}

} // end of namespace kvs
