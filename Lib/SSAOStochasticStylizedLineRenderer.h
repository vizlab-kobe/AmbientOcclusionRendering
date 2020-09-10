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
#include "SSAODrawable.h"


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
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;

private:
    kvs::UInt8 m_line_opacity; ///< line opacity

    kvs::Real32 m_radius_size;
    kvs::Real32 m_halo_size;
    BufferObject m_buffer_object;

    // SSAO
    SSAODrawable m_drawable;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

public:
    void setOpacity( const kvs::UInt8 opacity ){ m_line_opacity = opacity; }
    void setRadiusSize( const kvs::Real32 size ) { m_radius_size = size; }
    void setHaloSize( const kvs::Real32 size ) { m_halo_size = size; }
    kvs::UInt8 opacity() const { return m_line_opacity; }
    kvs::Real32 radiusSize() const { return m_radius_size; }
    kvs::Real32 haloSize() const { return m_halo_size; }
    void setSamplingSphereRadius( const float radius ) { m_drawable.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_drawable.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_drawable.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_drawable.numberOfSamplingPoints(); }

private:
    void create_buffer_object( const kvs::LineObject* line );
    void draw_buffer_object( const kvs::LineObject* line );
};

} // end of namespace AmbientOcclusionRendering
