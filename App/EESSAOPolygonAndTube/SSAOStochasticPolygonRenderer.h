#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/PolygonRenderer>
#include <kvs/Texture2D>
#include <kvs/StochasticRenderingEngine>
#include "StochasticRendererBase.h"


namespace local
{

/*===========================================================================*/
/**
 *  @brief  Stochastic polygon renderer class.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer : public local::StochasticRendererBase
{
    kvsModule( local::SSAOStochasticPolygonRenderer, Renderer );
    kvsModuleBaseClass( local::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticPolygonRenderer();

    void setEdgeFactor( const float factor );
    void setDepthOffset( const kvs::Vec2& offset );
    void setDepthOffset( const float factor, const float units = 0.0f );
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic polygon renderer.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer::Engine : public kvs::StochasticRenderingEngine
{
public:
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::glsl::PolygonRenderer::BufferObject;

    class RenderPass : public kvs::glsl::PolygonRenderer::RenderPass
    {
    private:
        using BaseRenderPass = kvs::glsl::PolygonRenderer::RenderPass;
        using Parent = BaseClass;
        const Parent* m_parent; ///< reference to the engine
    public:
        RenderPass( BufferObject& buffer_object, Parent* parent );
        void create( const kvs::Shader::ShadingModel& model, const bool enable );
        void setup( const kvs::Shader::ShadingModel& model );
    };

private:
    float m_edge_factor = 0.0f; ///< edge enhancement factor
    kvs::Vec2 m_depth_offset{ 0.0f, 0.0f }; ///< depth offset {factor, units}

    BufferObject m_buffer_object{}; ///< geometry buffer object
    RenderPass m_render_pass{ m_buffer_object, this }; ///< geometry pass for AO

public:
    Engine() = default;
    void release() { m_render_pass.release(); m_buffer_object.release(); }
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }
    void setDepthOffset( const kvs::Vec2& offset ) { m_depth_offset = offset; }
    void setDepthOffset( const float factor, const float units = 0.0f ) { m_depth_offset = kvs::Vec2( factor, units ); }

private:
    void create_buffer_object( const kvs::PolygonObject* polygon );
    void draw_buffer_object( const kvs::PolygonObject* polygon );
};

} // end of namespace local
