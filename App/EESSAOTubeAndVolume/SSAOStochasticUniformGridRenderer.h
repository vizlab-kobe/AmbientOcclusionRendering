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
#include <kvs/RayCastingRenderer>
#include "StochasticRendererBase.h"


namespace local
{

/*===========================================================================*/
/**
 *  @brief  Stochastic uniform grid renderer class.
 */
/*===========================================================================*/
class SSAOStochasticUniformGridRenderer : public local::StochasticRendererBase
{
    kvsModule( local::SSAOStochasticUniformGridRenderer, Renderer );
    kvsModuleBaseClass( local::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticUniformGridRenderer();
    void setSamplingStep( const float step );
    void setTransferFunction( const kvs::TransferFunction& transfer_function );
    const kvs::TransferFunction& transferFunction() const;
    float samplingStep() const;
    void setEdgeFactor( const float edge_factor );
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
    float m_step; ///< sampling step
    float m_edge_factor;

    // Transfer function
    bool m_transfer_function_changed; ///< flag for changin transfer function
    kvs::TransferFunction m_transfer_function; ///< transfer function
    kvs::Texture1D m_transfer_function_texture; ///< transfer function texture

    // Entry/exit framebuffer
    kvs::FrameBufferObject m_entry_exit_framebuffer; ///< framebuffer object for entry/exit point texture
    kvs::Texture2D m_entry_texture; ///< entry point texture
    kvs::Texture2D m_exit_texture; ///< exit point texture

    // Buffer object
    BoundingBufferObject m_bounding_cube_buffer;
//    kvs::Texture3D m_volume_texture; ///< volume data (3D texture)
    BufferObject m_volume_buffer;

    // Render pass
    BoundingRenderPass m_bounding_render_pass{ m_bounding_cube_buffer };
//    kvs::ProgramObject m_geom_pass_shader;
    RenderPass m_render_pass{ m_volume_buffer };

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingStep( const float step ) { m_step = step; }
    void setTransferFunction( const kvs::TransferFunction& transfer_function )
    {
        m_transfer_function = transfer_function;
        m_transfer_function_changed = true;
    }

    float samplingStep() const { return m_step; }
    const kvs::TransferFunction& transferFunction() const { return m_transfer_function; }
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }

private:
    void create_shader_program( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void update_shader_program( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    void setup_shader_program( const kvs::Shader::ShadingModel& shading_model, const kvs::ObjectBase* object, const kvs::Camera* camera, const kvs::Light* light );

    void create_transfer_function_texture();
    void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );

    void create_buffer_object( const kvs::StructuredVolumeObject* volume );
    void update_buffer_object( const kvs::StructuredVolumeObject* volume );
    void draw_buffer_object( const kvs::StructuredVolumeObject* volume );
};

} // end of namespace local
