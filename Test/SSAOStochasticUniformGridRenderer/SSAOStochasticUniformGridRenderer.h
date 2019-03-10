/*****************************************************************************/
/**
 *  @file   StochasticUniformGridRenderer.h
 *  @author Naoya Maeda, Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright (c) Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id$
 */
/*****************************************************************************/
#pragma once
#include <kvs/Module>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/FrameBufferObject>
#include <kvs/Texture2D>
#include <kvs/Texture3D>
#include <kvs/TransferFunction>
#include <kvs/StructuredVolumeObject>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Stochastic uniform grid renderer class.
 */
/*===========================================================================*/
class SSAOStochasticUniformGridRenderer : public kvs::StochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticUniformGridRenderer, Renderer );
    kvsModuleBaseClass( kvs::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticUniformGridRenderer();
    void setSamplingStep( const float step );
    void setTransferFunction( const kvs::TransferFunction& transfer_function );
    const kvs::TransferFunction& transferFunction() const;
    float samplingStep() const;
    void setSamplingSphereRadius( const float radius );
    void setNumberOfSamplingPoints( const size_t nsamples );
    kvs::Real32 samplingSphereRadius() const;
    size_t numberOfSamplingPoints() const;
};

/*===========================================================================*/
/**
 *  @brief  Engine class for stochastic uniform grid renderer.
 */
/*===========================================================================*/
class SSAOStochasticUniformGridRenderer::Engine : public kvs::StochasticRenderingEngine
{
private:
    size_t m_random_index; ///< index used for refering the random texture
    float m_step; ///< sampling step
    bool m_transfer_function_changed; ///< flag for changin transfer function
    kvs::TransferFunction m_transfer_function; ///< transfer function
    kvs::Texture1D m_transfer_function_texture; ///< transfer function texture
    kvs::Texture2D m_entry_texture; ///< entry point texture
    kvs::Texture2D m_exit_texture; ///< exit point texture
    kvs::Texture3D m_volume_texture; ///< volume data (3D texture)
    kvs::FrameBufferObject m_entry_exit_framebuffer; ///< framebuffer object for entry/exit point texture
    kvs::VertexBufferObjectManager m_bounding_cube_buffer; ///< bounding cube (VBO)

//    kvs::ProgramObject m_ray_casting_shader; ///< ray casting shader
    kvs::ProgramObject m_bounding_cube_shader; ///< bounding cube shader

    // Variables for SSAO
    kvs::ProgramObject m_shader_geom_pass; ///< shader program for geometry-pass (1st pass)
    kvs::ProgramObject m_shader_occl_pass; ///< shader program for occlusion-pass (2nd pass)
    kvs::FrameBufferObject m_framebuffer;
    kvs::Texture2D m_color_texture;
    kvs::Texture2D m_position_texture;
    kvs::Texture2D m_normal_texture;
    kvs::Texture2D m_depth_texture;
    kvs::Real32 m_sampling_sphere_radius;
    size_t m_nsamples;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingStep( const float step ) { m_step = step; }
    void setTransferFunction( const kvs::TransferFunction& transfer_function )
    {
        m_transfer_function = transfer_function;
        m_transfer_function_changed = true;
    }

    float samplingStep() const { return m_step; }
    const kvs::TransferFunction& transferFunction() const { return m_transfer_function; }

    void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }
    kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
    size_t numberOfSamplingPoints() const { return m_nsamples; }

private:
    void create_shader_program( const kvs::StructuredVolumeObject* volume );
    void create_volume_texture( const kvs::StructuredVolumeObject* volume );
    void create_transfer_function_texture();
    void create_bounding_cube_buffer( const kvs::StructuredVolumeObject* volume );
    void create_framebuffer( const size_t width, const size_t height );
    void create_sampling_points();
    void update_framebuffer( const size_t width, const size_t height );
    void draw_bounding_cube_buffer();
    void draw_quad();
    void render_geometry_pass( const kvs::StructuredVolumeObject* volume, const kvs::Camera* camera, const kvs::Light* light );
    void render_occlusion_pass( const kvs::StructuredVolumeObject* volume, const kvs::Light* light );
};

} // end of namespace AmbientOcclusionRendering
