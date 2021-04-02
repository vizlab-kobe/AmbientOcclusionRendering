/*****************************************************************************/
/**
 *  @file   StochasticRenderingCompositor.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOStochasticRenderingCompositor.h"
#include <kvs/Assert>
#include <kvs/OpenGL>
#include <kvs/PaintEvent>
#include <kvs/EventHandler>
#include <kvs/ScreenBase>
#include <kvs/Scene>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Background>
#include <kvs/ObjectManager>
#include <kvs/RendererManager>
#include <kvs/IDManager>
#include "SSAOStochasticRendererBase.h"
#include <kvs/ParticleBasedRenderer>

#include <kvs/ColorImage>
#include <kvs/GrayImage>
#include <kvs/String>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new StochasticRenderingCompositor class.
 *  @param  scene [in] pointer to the scene
 */
/*===========================================================================*/
SSAOStochasticRenderingCompositor::SSAOStochasticRenderingCompositor( kvs::Scene* scene ):
    m_scene( scene ),
    m_window_width( 0 ),
    m_window_height( 0 ),
    m_repetition_level( 1 ),
    m_coarse_level( 1 ),
    m_enable_lod( false ),
    m_enable_refinement( false ),
    m_shader( new kvs::Shader::Lambert() )
{
}

/*===========================================================================*/
/**
 *  @brief  Updates the scene.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::update()
{
    KVS_ASSERT( m_scene );

    kvs::OpenGL::WithPushedMatrix p( GL_MODELVIEW );
    p.loadIdentity();
    {
        m_scene->updateGLProjectionMatrix();
        m_scene->updateGLViewingMatrix();
        m_scene->updateGLLightParameters();
        m_scene->background()->apply();

        if ( m_scene->objectManager()->hasObject() )
        {
            this->draw();
        }
        else
        {
            m_scene->updateGLModelingMatrix();
        }
    }
}

/*===========================================================================*/
/**
 *  @brief  Draws the objects with stochastic renderers.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::draw()
{
    m_timer.start();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );

    this->check_window_created();
    this->check_window_resized();
    this->check_object_changed();

    // LOD control.
    size_t repetitions = m_repetition_level;
    kvs::Vec3 camera_position = m_scene->camera()->position();
    kvs::Vec3 light_position = m_scene->light()->position();
    kvs::Mat4 object_xform = this->object_xform();
    if ( m_camera_position != camera_position ||
         m_light_position != light_position ||
         m_object_xform != object_xform )
    {
        if ( m_enable_lod )
        {
            repetitions = m_coarse_level;
        }
        m_camera_position = camera_position;
        m_light_position = light_position;
        m_object_xform = object_xform;
        m_ensemble_buffer.clear();
    }

    // Setup engine.
    this->engines_setup();

    // Ensemble rendering.
    const bool reset_count = !m_enable_refinement;
    if ( reset_count ) m_ensemble_buffer.clear();
    for ( size_t i = 0; i < repetitions; i++ )
    {
        m_ensemble_buffer.bind();
        m_ao_buffer.bind();
        this->engines_draw();
        m_ao_buffer.unbind();
        m_ao_buffer.draw();
        m_ensemble_buffer.unbind();
        m_ensemble_buffer.add();
    }

    m_ensemble_buffer.draw();

    kvs::OpenGL::Finish();
    m_timer.stop();
}

/*===========================================================================*/
/**
 *  @brief  Check whether the window is created and initialize the parameters.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::check_window_created()
{
    const bool window_created = m_window_width == 0 && m_window_height == 0;
    if ( window_created )
    {
        const size_t width = m_scene->camera()->windowWidth();
        const size_t height = m_scene->camera()->windowHeight();
        m_window_width = width;
        m_window_height = height;

        const float dpr = m_scene->camera()->devicePixelRatio();
        const size_t framebuffer_width = static_cast<size_t>( width * dpr );
        const size_t framebuffer_height = static_cast<size_t>( height * dpr );
        m_ensemble_buffer.create( framebuffer_width, framebuffer_height );
        m_ensemble_buffer.clear();

        m_ao_buffer.createFramebuffer( framebuffer_width, framebuffer_height );
        m_ao_buffer.createShaderProgram( this->shader(), true );

        m_object_xform = this->object_xform();
        m_camera_position = m_scene->camera()->position();
        m_light_position = m_scene->light()->position();
        this->engines_create();
    }
}

/*===========================================================================*/
/**
 *  @brief  Check whether the window is resized and update the parameters.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::check_window_resized()
{
    const size_t width = m_scene->camera()->windowWidth();
    const size_t height = m_scene->camera()->windowHeight();
    const bool window_resized = m_window_width != width || m_window_height != height;
    if ( window_resized )
    {
        m_window_width = width;
        m_window_height = height;

        const float dpr = m_scene->camera()->devicePixelRatio();
        const size_t framebuffer_width = static_cast<size_t>( width * dpr );
        const size_t framebuffer_height = static_cast<size_t>( height * dpr );
        m_ensemble_buffer.release();
        m_ensemble_buffer.create( framebuffer_width, framebuffer_height );
        m_ensemble_buffer.clear();

        //m_ao_buffer.release();
        m_ao_buffer.updateFramebuffer( framebuffer_width, framebuffer_height );

        this->engines_update();
    }
}

/*===========================================================================*/
/**
 *  @brief  Check whether the object is changed and recreated the engine.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::check_object_changed()
{
//    typedef StochasticRendererBase Renderer;
//    using Renderer = local::StochasticRendererBase;
    using Renderer = SSAOStochasticRendererBase;

    const size_t size = m_scene->IDManager()->size();
    for ( size_t i = 0; i < size; i++ )
    {
        kvs::IDManager::IDPair id = m_scene->IDManager()->id( i );
        kvs::ObjectBase* object = m_scene->objectManager()->object( id.first );
        kvs::RendererBase* renderer = m_scene->rendererManager()->renderer( id.second );
        if ( Renderer* stochastic_renderer = Renderer::DownCast( renderer ) )
        {
            const bool object_changed = stochastic_renderer->engine().object() != object;
            if ( object_changed )
            {
                m_ensemble_buffer.clear();
                if ( stochastic_renderer->engine().object() ) stochastic_renderer->engine().release();
                stochastic_renderer->engine().setDepthTexture( m_ensemble_buffer.currentDepthTexture() );
                stochastic_renderer->engine().setShader( &stochastic_renderer->shader() );
                stochastic_renderer->engine().setRepetitionLevel( m_repetition_level );
                stochastic_renderer->engine().setShadingEnabled( stochastic_renderer->isShadingEnabled() );

                kvs::OpenGL::PushMatrix();
                m_scene->updateGLModelingMatrix( object );
                stochastic_renderer->engine().create( object, m_scene->camera(), m_scene->light() );
                kvs::OpenGL::PopMatrix();
            }
        }
    }
}

/*===========================================================================*/
/**
 *  @brief  Returns the xform matrix of the active object.
 *  @return xform matrix
 */
/*===========================================================================*/
kvs::Mat4 SSAOStochasticRenderingCompositor::object_xform()
{
    return m_scene->objectManager()->hasActiveObject() ?
        m_scene->objectManager()->activeObject()->xform().toMatrix() :
        m_scene->objectManager()->xform().toMatrix();
}

/*===========================================================================*/
/**
 *  @brief  Calls the create method of each engine.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::engines_create()
{
    typedef SSAOStochasticRendererBase Renderer;

    const size_t size = m_scene->IDManager()->size();
    for ( size_t i = 0; i < size; i++ )
    {
        kvs::IDManager::IDPair id = m_scene->IDManager()->id( i );
        kvs::ObjectBase* object = m_scene->objectManager()->object( id.first );
        kvs::RendererBase* renderer = m_scene->rendererManager()->renderer( id.second );
        if ( Renderer* stochastic_renderer = Renderer::DownCast( renderer ) )
        {
            stochastic_renderer->engine().setDepthTexture( m_ensemble_buffer.currentDepthTexture() );
            stochastic_renderer->engine().setShader( &stochastic_renderer->shader() );
            stochastic_renderer->engine().setRepetitionLevel( m_repetition_level );
            stochastic_renderer->engine().setShadingEnabled( stochastic_renderer->isShadingEnabled() );

            kvs::OpenGL::PushMatrix();
            m_scene->updateGLModelingMatrix( object );
            stochastic_renderer->engine().create( object, m_scene->camera(), m_scene->light() );
            kvs::OpenGL::PopMatrix();
        }
    }
}

/*===========================================================================*/
/**
 *  @brief  Calls the update method of each engine.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::engines_update()
{
    m_ao_buffer.updateShaderProgram( this->shader(), true );

    using Renderer = SSAOStochasticRendererBase;
    auto* camera = m_scene->camera();
    auto* light = m_scene->light();

    const size_t size = m_scene->IDManager()->size();
    for ( size_t i = 0; i < size; i++ )
    {
        kvs::IDManager::IDPair id = m_scene->IDManager()->id( i );
        kvs::ObjectBase* object = m_scene->objectManager()->object( id.first );
        kvs::RendererBase* renderer = m_scene->rendererManager()->renderer( id.second );
        if ( Renderer* stochastic_renderer = Renderer::DownCast( renderer ) )
        {
            kvs::OpenGL::PushMatrix();
            m_scene->updateGLModelingMatrix( object );
            stochastic_renderer->engine().update( object, camera, light );
            kvs::OpenGL::PopMatrix();
        }
    }
}

/*===========================================================================*/
/**
 *  @brief  Calls the setup method of each engine.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::engines_setup()
{
    m_ao_buffer.setupShaderProgram( this->shader() );

    using Renderer = SSAOStochasticRendererBase;
    auto* camera = m_scene->camera();
    auto* light = m_scene->light();
    const bool reset_count = !m_enable_refinement;

    const size_t size = m_scene->IDManager()->size();
    for ( size_t i = 0; i < size; i++ )
    {
        kvs::IDManager::IDPair id = m_scene->IDManager()->id( i );
        kvs::ObjectBase* object = m_scene->objectManager()->object( id.first );
        kvs::RendererBase* renderer = m_scene->rendererManager()->renderer( id.second );
        if ( Renderer* stochastic_renderer = Renderer::DownCast( renderer ) )
        {
            kvs::OpenGL::PushMatrix();
            m_scene->updateGLModelingMatrix( object );
            if ( reset_count ) stochastic_renderer->engine().resetRepetitions();
            stochastic_renderer->engine().setup( object, camera, light );
            kvs::OpenGL::PopMatrix();
        }
    }
}

/*===========================================================================*/
/**
 *  @brief  Calls the draw method of each engine.
 */
/*===========================================================================*/
void SSAOStochasticRenderingCompositor::engines_draw()
{
    typedef SSAOStochasticRendererBase Renderer;

    kvs::Camera* camera = m_scene->camera();
    kvs::Light* light = m_scene->light();

    const size_t size = m_scene->IDManager()->size();
    for ( size_t i = 0; i < size; i++ )
    {
        kvs::IDManager::IDPair id = m_scene->IDManager()->id( i );
        kvs::ObjectBase* object = m_scene->objectManager()->object( id.first );
        kvs::RendererBase* renderer = m_scene->rendererManager()->renderer( id.second );
        if ( Renderer* stochastic_renderer = Renderer::DownCast( renderer ) )
        {
            if ( object->isShown() )
            {
                kvs::OpenGL::PushMatrix();
                m_scene->updateGLModelingMatrix( object );
                stochastic_renderer->engine().draw( object, camera, light );
                stochastic_renderer->engine().countRepetitions();

                /*
                auto width = camera->windowWidth();
                auto height = camera->windowHeight();
                kvs::ValueArray<kvs::Real32> buffer( width * height );
                kvs::OpenGL::ReadPixels( 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer.data() );
                kvs::GrayImage depth_image( width, height, buffer );
                depth_image.write( "output_" + kvs::String::From( i, 2, '0' ) + ".bmp" );
                */

                kvs::OpenGL::PopMatrix();
            }
        }
    }
}

} // end of namespace local
