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
 *  @brief  Stochastic polygon renderer class.
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
    void setPolygonOffset( const float offset );
    void setEdgeFactor( const float factor );
    void setSamplingSphereRadius( const float radius );
    void setNumberOfSamplingPoints( const size_t nsamples );
    kvs::Real32 samplingSphereRadius() const;
    size_t numberOfSamplingPoints() const;
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic polygon renderer.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::glsl::PolygonRenderer::BufferObject;

private:
    float m_polygon_offset; ///< polygon offset
    float m_edge_factor; ///< edge enhancement factor
    BufferObject m_buffer_object; ///< geometry buffer object
    AmbientOcclusionBuffer m_ao_buffer; ///< ambient occlusion buffer

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setPolygonOffset( const float offset ) { m_polygon_offset = offset; }
    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }

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
