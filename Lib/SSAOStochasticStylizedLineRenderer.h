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
#include "AmbientOcclusionBuffer.h"


namespace AmbientOcclusionRendering
{

class SSAOStochasticStylizedLineRenderer : public kvs::StochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticStylizedLineRenderer, Renderer );
    kvsModuleBaseClass( kvs::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticStylizedLineRenderer();
    void setEdgeFactor( const float factor );
    /*KVS_DEPRECATED*/ void setOpacity( const kvs::UInt8 opacity );
    void setRadiusSize( const kvs::Real32 size );
    void setHaloSize( const kvs::Real32 size );
    void setSamplingSphereRadius( const float radius );
    void setNumberOfSamplingPoints( const size_t nsamples );
    /*KVS_DEPRECATED*/ kvs::UInt8 opacity() const;
    kvs::Real32 radiusSize() const;
    kvs::Real32 haloSize() const;
    kvs::Real32 samplingSphereRadius();
    size_t numberOfSamplingPoints();
};

class SSAOStochasticStylizedLineRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;

private:
    float m_edge_factor; ///< edge enhancement factor
    kvs::UInt8 m_line_opacity; ///< line opacity
    kvs::Real32 m_radius_size; ///< radius size of tube
    kvs::Real32 m_halo_size; ///< halo size of tube
    BufferObject m_buffer_object; ///< geometry buffer object
    AmbientOcclusionBuffer m_ao_buffer; ///< ambient occlusion buffer

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setEdgeFactor( const float factor ) { m_edge_factor = factor; }
    void setOpacity( const kvs::UInt8 opacity ){ m_line_opacity = opacity; }
    void setRadiusSize( const kvs::Real32 size ) { m_radius_size = size; }
    void setHaloSize( const kvs::Real32 size ) { m_halo_size = size; }
    kvs::UInt8 opacity() const { return m_line_opacity; }
    kvs::Real32 radiusSize() const { return m_radius_size; }
    kvs::Real32 haloSize() const { return m_halo_size; }
    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }

private:
    void create_buffer_object( const kvs::LineObject* line );
    void update_buffer_object( const kvs::LineObject* line );
    void draw_buffer_object( const kvs::LineObject* line );
};

} // end of namespace AmbientOcclusionRendering
