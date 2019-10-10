#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/Texture2D>
#include "SSAOStochasticRenderingEngine.h"
#include "SSAOStochasticRendererBase.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Stochastic polygon renderer class.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer : public AmbientOcclusionRendering::SSAOStochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticPolygonRenderer, Renderer );
    kvsModuleBaseClass( AmbientOcclusionRendering::SSAOStochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticPolygonRenderer();
    void setPolygonOffset( const float polygon_offset );
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic polygon renderer.
 */
/*===========================================================================*/
class SSAOStochasticPolygonRenderer::Engine : public AmbientOcclusionRendering::SSAOStochasticRenderingEngine
{
private:
    bool m_has_normal; ///< check flag for the normal array
    bool m_has_connection; ///< check flag for the connection array
    float m_polygon_offset; ///< polygon offset
    kvs::VertexBufferObjectManager m_vbo_manager; ///< vertex buffer object manager

    // Variables for SSAO
    kvs::ProgramObject m_shader_geom_pass; ///< shader program for geometry-pass (1st pass)

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
	void setPolygonOffset( const float offset ) { m_polygon_offset = offset; }

private:
    void create_shader_program();
    void create_buffer_object( const kvs::PolygonObject* polygon );
};

} // end of namespace AmbientOcclusionRendering
