#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/PolygonRenderer>
#include <kvs/Texture2D>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>
#include "AmbientOcclusionBuffer.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  SSAO Stochastic polygon renderer class.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer : public kvs::StochasticRendererBase
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

    // AO buffer properties
    void setSamplingSphereRadius( const float radius );
    void setNumberOfSamplingPoints( const size_t nsamples );
    kvs::Real32 samplingSphereRadius() const;
    size_t numberOfSamplingPoints() const;
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

private:
    float m_edge_factor = 0.0f; ///< edge enhancement factor
    kvs::Vec2 m_depth_offset{ 0.0f, 0.0f }; ///< depth offset {factor, units}

    BufferObject m_buffer_object; ///< geometry buffer object
    AmbientOcclusionBuffer m_ao_buffer; ///< ambient occlusion buffer

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
    void setDepthOffset( const float factor, const float units = 0.0f ) { m_depth_offset = kvs::Vec2( factor, units ); }

    // AO buffer properties
    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }

private:
    void create_buffer_object( const kvs::PolygonObject* polygon );
    void update_buffer_object( const kvs::PolygonObject* polygon );
    void draw_buffer_object( const kvs::PolygonObject* polygon );
};

} // end of namespace AmbientOcclusionRendering
