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
#include <AmbientOcclusionRendering/Lib/AmbientOcclusionBuffer.h>


namespace local
{

class SSAOStochasticTubeRenderer : public kvs::StochasticRendererBase
{
    kvsModule( local::SSAOStochasticTubeRenderer, Renderer );
    kvsModuleBaseClass( kvs::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticTubeRenderer();
    void setTransferFunction( const kvs::TransferFunction& tfunc );
    void setRadiusSize( const kvs::Real32 size );
    void setHaloSize( const kvs::Real32 size );
    void setSamplingSphereRadius( const float radius );
    void setNumberOfSamplingPoints( const size_t nsamples );
    const kvs::TransferFunction& transferFunction() const;
    kvs::Real32 radiusSize() const;
    kvs::Real32 haloSize() const;
    kvs::Real32 samplingSphereRadius() const;
    size_t numberOfSamplingPoints() const;
    void setEdgeFactor( const float edge_factor );
};

class SSAOStochasticTubeRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;

private:
    kvs::Real32 m_radius_size;
    kvs::Real32 m_halo_size;
    BufferObject m_buffer_object;

    bool m_tfunc_changed; ///< flag for changing transfer function
    kvs::TransferFunction m_tfunc; ///< transfer function
    kvs::Texture1D m_tfunc_texture; ///< transfer function texture
    AmbientOcclusionRendering::AmbientOcclusionBuffer m_ao_buffer;
    float m_edge_factor;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

public:
    void setTransferFunction( const kvs::TransferFunction& tfunc ) { m_tfunc = tfunc; m_tfunc_changed = true; }
    void setRadiusSize( const kvs::Real32 size ) { m_radius_size = size; }
    void setHaloSize( const kvs::Real32 size ) { m_halo_size = size; }
    const kvs::TransferFunction& transferFunction() const { return m_tfunc; }
    kvs::Real32 radiusSize() const { return m_radius_size; }
    kvs::Real32 haloSize() const { return m_halo_size; }
    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }

private:
    void create_buffer_object( const kvs::LineObject* line );
    void create_transfer_function_texture();
    void draw_buffer_object( const kvs::LineObject* line );
};

} // end of namespace local
