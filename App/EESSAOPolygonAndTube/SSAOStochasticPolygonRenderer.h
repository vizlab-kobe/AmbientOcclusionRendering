#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/PolygonRenderer>
#include <kvs/Texture2D>
//#include "StochasticRenderingEngine.h"
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
    void setPolygonOffset( const float polygon_offset );
    void setEdgeFactor( const float edge_control );
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic polygon renderer.
 */
/*===========================================================================*/
//class SSAOStochasticPolygonRenderer::Engine : public local::StochasticRenderingEngine
class SSAOStochasticPolygonRenderer::Engine : public kvs::StochasticRenderingEngine
{
    using BufferObject = kvs::glsl::PolygonRenderer::BufferObject;

private:
    float m_polygon_offset; ///< polygon offset
    float m_edge_factor; ///< edge enhancement control factor
    BufferObject m_buffer_object;
    kvs::ProgramObject m_geom_pass_shader;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    
    void setPolygonOffset( const float offset ) { m_polygon_offset = offset; }
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }
    
private:
    void create_geometry_shader_program();
    void create_buffer_object( const kvs::PolygonObject* polygon );
    void draw_buffer_object( const kvs::PolygonObject* polygon );
};

} // end of namespace local
