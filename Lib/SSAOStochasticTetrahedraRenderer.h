/*****************************************************************************/
/**
 *  @file   StochasticTetrahedraRenderer.h
 *  @author Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 * References:
 * [1] Naohisa Sakamoto, Koji Koyamada, "A Stochastic Approach for Rendering
 *     Multiple Irregular Volumes", In Proc. of IEEE Pacific Visualization
 *     2014 (VisNotes), pp.272-276, 2014.3.
 */
/*****************************************************************************/
#pragma once
#include <kvs/Module>
#include <kvs/TransferFunction>
#include <kvs/Texture1D>
#include <kvs/Texture2D>
#include <kvs/ProgramObject>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>
#include <kvs/StochasticTetrahedraRenderer>
#include "SSAOStochasticRendererBase.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Stochastic tetrahedra renderer class.
 */
/*===========================================================================*/
class SSAOStochasticTetrahedraRenderer : public SSAOStochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer, Renderer );
    kvsModuleBaseClass( SSAOStochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticTetrahedraRenderer();
    virtual ~SSAOStochasticTetrahedraRenderer() {}

    void setEdgeFactor( const float factor );
    void setSamplingStep( const float sampling_step );
    void setTransferFunction( const kvs::TransferFunction& transfer_function );
    const kvs::TransferFunction& transferFunction() const;
    float samplingStep() const;
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic polygon renderer.
 */
/*===========================================================================*/
class SSAOStochasticTetrahedraRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using TetEngine = kvs::StochasticTetrahedraRenderer::Engine;
public:
    using BaseClass = kvs::StochasticRenderingEngine;
    using TransferFunctionBuffer = TetEngine::TransferFunctionBuffer;
    using PreIntegrationBuffer = TetEngine::PreIntegrationBuffer;
    using DecompositionBuffer = TetEngine::DecompositionBuffer;
    using BufferObject = TetEngine::BufferObject;
    using RenderPass = TetEngine::RenderPass;

private:
    bool m_transfer_function_changed = true; ///< flag for changin transfer function
    kvs::TransferFunction m_transfer_function{}; ///< transfer function
    TransferFunctionBuffer m_transfer_function_buffer{}; ///< transfer function buffer
    PreIntegrationBuffer m_preintegration_buffer{}; ///< pre-integration buffer
    DecompositionBuffer m_decomposition_buffer{}; ///< decomposition buffer
    BufferObject m_buffer_object{ this }; ///< buffer object
    RenderPass m_render_pass{ m_buffer_object }; ///< render pass (geometry pass for SSAO)
    kvs::Real32 m_edge_factor = 0.0f; ///< edge enhancement factor

public:
    Engine();
    virtual ~Engine() { this->release(); }
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setSamplingStep( const float step ) { m_render_pass.setSamplingStep( step ); }
    void setTransferFunction( const kvs::TransferFunction& transfer_function )
    {
        m_transfer_function = transfer_function;
        m_transfer_function_changed = true;
    }

    float samplingStep() const { return m_render_pass.samplingStep(); }
    const kvs::TransferFunction& transferFunction() const { return m_transfer_function; }

private:
    void create_transfer_function_texture();
    void update_transfer_function_texture();

    void create_decomposition_texture();

    void create_buffer_object( const kvs::UnstructuredVolumeObject* volume );
    void update_buffer_object( const kvs::UnstructuredVolumeObject* volume );
    void draw_buffer_object( const kvs::UnstructuredVolumeObject* volume );
};

} // end of namespace AmbientOcclusionRendering
