/*****************************************************************************/
/**
 *  @file   StochasticUniformGridRenderer.h
 *  @author Naoya Maeda, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/Module>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/FrameBufferObject>
#include <kvs/Texture2D>
#include <kvs/Texture3D>
#include <kvs/TransferFunction>
#include <kvs/StructuredVolumeObject>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>
#include <kvs/RayCastingRenderer>
#include "SSAOStochasticRendererBase.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Stochastic uniform grid renderer class.
 */
/*===========================================================================*/
class SSAOStochasticUniformGridRenderer : public SSAOStochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticUniformGridRenderer, Renderer );
    kvsModuleBaseClass( SSAOStochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticUniformGridRenderer();
    virtual ~SSAOStochasticUniformGridRenderer() {}

    void setEdgeFactor( const float factor );
    void setSamplingStep( const float step );
    void setTransferFunction( const kvs::TransferFunction& transfer_function );
    const kvs::TransferFunction& transferFunction() const;
    float samplingStep() const;
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic uniform grid renderer.
 */
/*===========================================================================*/
class SSAOStochasticUniformGridRenderer::Engine : public kvs::StochasticRenderingEngine
{
public:
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::glsl::RayCastingRenderer::BufferObject;
    using RenderPass = kvs::glsl::RayCastingRenderer::RenderPass;
    using BoundingBufferObject = kvs::glsl::RayCastingRenderer::BoundingBufferObject;
    using BoundingRenderPass = kvs::glsl::RayCastingRenderer::BoundingRenderPass;

private:
    // Variables
    float m_edge_factor = 0.0f; ///< edge enhancement factor
    float m_step = 0.5f; ///< sampling step

    // Transfer function
    bool m_transfer_function_changed = true; ///< flag for changin transfer function
    kvs::TransferFunction m_transfer_function{}; ///< transfer function
    kvs::Texture1D m_transfer_function_texture{}; ///< transfer function texture

    // Exit/entry framebuffer
    kvs::FrameBufferObject m_entry_exit_framebuffer{}; ///< framebuffer object for entry/exit point texture
    kvs::Texture2D m_entry_texture{}; ///< entry point texture
    kvs::Texture2D m_exit_texture{}; ///< exit point texture

    // Buffer objects and render passes
    BufferObject m_volume_buffer{}; ///< volume buffer object
    RenderPass m_render_pass{ m_volume_buffer };

    BoundingBufferObject m_bounding_cube_buffer{}; ///< bounding cube buffer
    BoundingRenderPass m_bounding_render_pass{ m_bounding_cube_buffer };

public:
    Engine();
    virtual ~Engine() { this->release(); }
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setSamplingStep( const float step ) { m_step = step; }
    void setTransferFunction( const kvs::TransferFunction& transfer_function )
    {
        m_transfer_function = transfer_function;
        m_transfer_function_changed = true;
    }

    float samplingStep() const { return m_step; }
    const kvs::TransferFunction& transferFunction() const { return m_transfer_function; }

private:
    void create_transfer_function_texture();
    void update_transfer_function_texture();

    void create_shader_program( const kvs::StructuredVolumeObject* volume );
    void update_shader_program( const kvs::StructuredVolumeObject* volume );
    void setup_shader_program(
        const kvs::Shader::ShadingModel& shading_model,
        const kvs::ObjectBase* object,
        const kvs::Camera* camera,
        const kvs::Light* light );

    void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );

    void create_buffer_object( const kvs::StructuredVolumeObject* volume );
    void update_buffer_object( const kvs::StructuredVolumeObject* volume );
    void draw_buffer_object( const kvs::StructuredVolumeObject* volume );
};

} // end of namespace AmbientOcclusionRendering
