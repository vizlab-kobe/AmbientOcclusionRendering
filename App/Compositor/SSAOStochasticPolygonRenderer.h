#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/PolygonRenderer>
#include <kvs/Texture2D>
#include "StochasticRenderingEngine.h"
#include "StochasticRendererBase.h"
#include "AmbientOcclusionBuffer.h"


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
    void setPolygonOffset( const float polygon_offset );
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
class SSAOStochasticPolygonRenderer::Engine : public local::StochasticRenderingEngine
{
    using BufferObject = kvs::glsl::PolygonRenderer::BufferObject;

private:
    float m_polygon_offset; ///< polygon offset
    BufferObject m_buffer_object;
    local::AmbientOcclusionBuffer m_ao_buffer;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void create_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    
    void setPolygonOffset( const float offset ) { m_polygon_offset = offset; }
    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }
    //kvs::ProgramObject& geometryPassShader() { return m_geom_pass_shader; }
    
private:
    void create_shader_program();
    void create_buffer_object( const kvs::PolygonObject* polygon );
    void draw_buffer_object( const kvs::PolygonObject* polygon );
    void create_geom_shader_program();
    void create_buffer_object_c( const kvs::PolygonObject* polygon );
    void draw_buffer_object_c( const kvs::PolygonObject* polygon );
};

} // end of namespace local
