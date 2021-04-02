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
#include <kvs/StylizedLineRenderer>
#include "SSAOStochasticRendererBase.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  SSAO stochastic stylized line renderer class.
 */
/*===========================================================================*/
class SSAOStochasticStylizedLineRenderer : public SSAOStochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticStylizedLineRenderer, Renderer );
    kvsModuleBaseClass( SSAOStochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticStylizedLineRenderer();
    virtual ~SSAOStochasticStylizedLineRenderer() {}

    void setEdgeFactor( const float factor );
    /*KVS_DEPRECATED*/ void setOpacity( const kvs::UInt8 opacity );
    void setRadiusSize( const kvs::Real32 size );
    void setHaloSize( const kvs::Real32 size );
    /*KVS_DEPRECATED*/ kvs::UInt8 opacity() const;
    kvs::Real32 radiusSize() const;
    kvs::Real32 haloSize() const;
};

/*===========================================================================*/
/**
 *  @brief  Engine class for SSAO stochastic stylized line renderer.
 */
/*===========================================================================*/
class SSAOStochasticStylizedLineRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;
    using RenderPass = kvs::StylizedLineRenderer::RenderPass;

private:
    float m_edge_factor = 0.0f; ///< edge enhancement factor
    kvs::UInt8 m_line_opacity = 255; ///< line opacity

    BufferObject m_buffer_object{}; ///< geometry buffer object
    RenderPass m_render_pass{ m_buffer_object }; ///< geometry pass

public:
    Engine();
    virtual ~Engine() { this-> release(); }

    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setOpacity( const kvs::UInt8 opacity ){ m_line_opacity = opacity; }
    void setRadiusSize( const kvs::Real32 size ) { m_render_pass.setRadiusSize( size ); }
    void setHaloSize( const kvs::Real32 size ) { m_render_pass.setHaloSize( size ); }
    kvs::UInt8 opacity() const { return m_line_opacity; }
    kvs::Real32 radiusSize() const { return m_render_pass.radiusSize(); }
    kvs::Real32 haloSize() const { return m_render_pass.haloSize(); }

private:
    void create_buffer_object( const kvs::LineObject* line );
    void update_buffer_object( const kvs::LineObject* line );
    void draw_buffer_object( const kvs::LineObject* line );
};

} // end of namespace AmbientOcclusionRendering
