#pragma once
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/Texture2D>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>
#include "SSAODrawable.h"


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
class SSAOStochasticPolygonRenderer::Engine : public kvs::StochasticRenderingEngine
{
private:
    bool m_has_normal; ///< check flag for the normal array
    bool m_has_connection; ///< check flag for the connection array
    float m_polygon_offset; ///< polygon offset
    kvs::VertexBufferObjectManager m_vbo_manager; ///< vertex buffer object manager
//    kvs::ProgramObject m_shader_program; ///< shader program

    // Variables for SSAO
    /*
    kvs::ProgramObject m_shader_geom_pass; ///< shader program for geometry-pass (1st pass)
    kvs::ProgramObject m_shader_occl_pass; ///< shader program for occlusion-pass (2nd pass)
    kvs::FrameBufferObject m_framebuffer;
    kvs::Texture2D m_color_texture;
    kvs::Texture2D m_position_texture;
    kvs::Texture2D m_normal_texture;
    kvs::Texture2D m_depth_texture;
    kvs::Real32 m_sampling_sphere_radius;
    size_t m_nsamples;
    */
    SSAODrawable m_drawable;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setPolygonOffset( const float offset ) { m_polygon_offset = offset; }
//    void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
//    void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }
//    kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
//    size_t numberOfSamplingPoints() const { return m_nsamples; }
    void setSamplingSphereRadius( const float radius ) { m_drawable.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_drawable.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_drawable.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_drawable.numberOfSamplingPoints(); }

private:
//    void create_shader_program();
    void create_buffer_object( const kvs::PolygonObject* polygon );
//    void create_framebuffer( const size_t width, const size_t height );
//    void create_sampling_points();
//    void update_framebuffer( const size_t width, const size_t height );
    void render_geometry_pass( const kvs::PolygonObject* polygon );
    void render_occlusion_pass();
};

} // end of namespace AmbientOcclusionRendering
