#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/PolygonRenderer>
#include <kvs/Texture2D>
#include <kvs/StochasticRenderingEngine>
#include "SSAOStochasticRendererBase.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  SSAO Stochastic polygon renderer class.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer : public SSAOStochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticPolygonRenderer, Renderer );
    kvsModuleBaseClass( kvs::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticPolygonRenderer();
    virtual ~SSAOStochasticPolygonRenderer() {}

    void setEdgeFactor( const float factor );
    void setDepthOffset( const kvs::Vec2& offset );
    void setDepthOffset( const float factor, const float units = 0.0f );
};

/*===========================================================================*/
/**
 *  @brief  Engine class for SSAO stochastic polygon renderer.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::glsl::PolygonRenderer::BufferObject;
    using RenderPass = kvs::glsl::PolygonRenderer::RenderPass;

private:
    float m_edge_factor = 0.0f; ///< edge enhancement factor
    kvs::Vec2 m_depth_offset{ 0.0f, 0.0f }; ///< depth offset {factor, units}

    BufferObject m_buffer_object{}; ///< geometry buffer object
    RenderPass m_render_pass{ m_buffer_object }; ///< geometry pass

public:
    Engine();
    virtual ~Engine() { this->release(); }

    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setDepthOffset( const kvs::Vec2& offset ) { m_depth_offset = offset; }
    void setDepthOffset( const float factor, const float units = 0.0f )
    {
        m_depth_offset = kvs::Vec2( factor, units );
    }

private:
    void create_buffer_object( const kvs::PolygonObject* polygon );
    void update_buffer_object( const kvs::PolygonObject* polygon );
    void draw_buffer_object( const kvs::PolygonObject* polygon );
};

} // end of namespace AmbientOcclusionRendering
