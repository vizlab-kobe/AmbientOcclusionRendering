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
    using BaseClass = kvs::StochasticRenderingEngine;

private:
    float m_edge_factor; ///< edge enhancement factor

    // Transfer function
    bool m_transfer_function_changed; ///< flag for changin transfer function
    kvs::TransferFunction m_transfer_function; ///< transfer function
    kvs::Texture2D m_preintegration_texture; ///< pre-integration texture
    kvs::Texture2D m_decomposition_texture; ///< texture for the tetrahedral decomposition
    kvs::Texture1D m_transfer_function_texture; ///< transfer function texture
    kvs::Texture1D m_T_texture; ///< T function for pre-integration
    kvs::Texture1D m_inv_T_texture; ///< inverse function of T for pre-integration

    // Buffer object
    kvs::VertexBufferObjectManager m_vbo_manager; ///< vertex buffer object manager

    float m_sampling_step; ///< sampling step
    float m_maxT; ///< maximum value of T

    // Render pass (geom pass)
    std::string m_vert_shader_file;
    std::string m_frag_shader_file;
    std::string m_geom_shader_file;
    kvs::ProgramObject m_shader_program; ///< shader program

public:
    Engine();
    virtual ~Engine() { this->release(); }
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setSamplingStep( const float sampling_step ) { m_sampling_step = sampling_step; }
    void setTransferFunction( const kvs::TransferFunction& transfer_function )
    {
        m_transfer_function = transfer_function;
        m_transfer_function_changed = true;
    }

    float samplingStep() const { return m_sampling_step; }
    const kvs::TransferFunction& transferFunction() const { return m_transfer_function; }

private:
    void create_preintegration_texture();
    void create_decomposition_texture();

    void create_buffer_object( const kvs::UnstructuredVolumeObject* volume );
    void update_buffer_object( const kvs::UnstructuredVolumeObject* volume );
    void draw_buffer_object( const kvs::UnstructuredVolumeObject* volume );
};

} // end of namespace AmbientOcclusionRendering
