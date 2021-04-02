#pragma once
#include <kvs/DebugNew>
#include <kvs/Module>
#include <kvs/LineObject>
#include <kvs/LineRenderer>
#include <kvs/Shader>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/Texture2D>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>
#include <kvs/TransferFunction>
#include <kvs/StylizedLineRenderer>
#include "SSAOStochasticRendererBase.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  SSAO stochastic tube renderer class.
 */
/*===========================================================================*/
class SSAOStochasticTubeRenderer : public SSAOStochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticTubeRenderer, Renderer );
    kvsModuleBaseClass( SSAOStochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticTubeRenderer();
    virtual ~SSAOStochasticTubeRenderer() {}

    void setEdgeFactor( const float factor );
    void setTransferFunction( const kvs::TransferFunction& tfunc );
    void setRadiusSize( const kvs::Real32 size );
    void setHaloSize( const kvs::Real32 size );
    const kvs::TransferFunction& transferFunction() const;
    kvs::Real32 radiusSize() const;
    kvs::Real32 haloSize() const;
};

/*===========================================================================*/
/**
 *  @brief  Engine class for SSAO stochastic tube renderer.
 */
/*===========================================================================*/
class SSAOStochasticTubeRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;
    using RenderPass = kvs::StylizedLineRenderer::RenderPass;

private:
    float m_edge_factor = 0.0f; ///< edge enhancement factor

    bool m_tfunc_changed = true; ///< flag for changing transfer function
    kvs::TransferFunction m_tfunc{}; ///< transfer function
    kvs::Texture1D m_tfunc_texture{}; ///< transfer function texture

    BufferObject m_buffer_object{};
    RenderPass m_render_pass{ m_buffer_object };

public:
    Engine();
    virtual ~Engine() { this->release(); }

    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setTransferFunction( const kvs::TransferFunction& tfunc ) { m_tfunc = tfunc; m_tfunc_changed = true; }
    void setRadiusSize( const kvs::Real32 size ) { m_render_pass.setRadiusSize( size ); }
    void setHaloSize( const kvs::Real32 size ) { m_render_pass.setHaloSize( size ); }
    const kvs::TransferFunction& transferFunction() const { return m_tfunc; }
    kvs::Real32 radiusSize() const { return m_render_pass.radiusSize(); }
    kvs::Real32 haloSize() const { return m_render_pass.haloSize(); }

private:
    void create_transfer_function_texture();
    void update_transfer_function_texture();

    void create_buffer_object( const kvs::LineObject* line );
    void update_buffer_object( const kvs::LineObject* line );
    void draw_buffer_object( const kvs::LineObject* line );
};

} // end of namespace AmbientOcclusionRendering
