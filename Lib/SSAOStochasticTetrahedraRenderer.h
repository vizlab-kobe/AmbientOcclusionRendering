/*****************************************************************************/
/**
 *  @file   StochasticTetrahedraRenderer.h
 *  @author Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright (c) Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id$
 */
/*----------------------------------------------------------------------------
 *
 * References:
 * [1] Naohisa Sakamoto, Koji Koyamada, "A Stochastic Approach for Rendering
 *     Multiple Irregular Volumes", In Proc. of IEEE Pacific Visualization
 *     2014 (VisNotes), pp.272-276, 2014.3.
 */
/*****************************************************************************/
#pragma once
#include <kvs/Module>
#include <kvs/TransferFunction>
#include <kvs/Texture1D>
#include <kvs/Texture2D>
#include <kvs/ProgramObject>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/StochasticRenderingEngine>
#include <kvs/StochasticRendererBase>
#include "SSAODrawable.h"


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Stochastic tetrahedra renderer class.
 */
/*===========================================================================*/
class SSAOStochasticTetrahedraRenderer : public kvs::StochasticRendererBase
{
    kvsModule( AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer, Renderer );
    kvsModuleBaseClass( kvs::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticTetrahedraRenderer();
    void setTransferFunction( const kvs::TransferFunction& transfer_function );
    void setSamplingStep( const float sampling_step );
    const kvs::TransferFunction& transferFunction() const;
    float samplingStep() const;
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
class SSAOStochasticTetrahedraRenderer::Engine : public kvs::StochasticRenderingEngine
{
private:
    class Drawable : public SSAODrawable
    {
    private:
        const Engine* m_engine;
        std::string m_geom_pass_shader_geom_file;
    public:
        Drawable( const Engine* engine ) : SSAODrawable(), m_engine( engine ) {}
        const std::string& geometryPassGeometryShaderFile() const { return m_geom_pass_shader_geom_file; }
        void setGeometryPassShaderFiles( const std::string& vert_file, const std::string& geom_file, const std::string& frag_file );
        void createShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
        void updateShaderProgram( const kvs::Shader::ShadingModel& shading_model, const bool shading_enabled );
    };

private:
    bool m_transfer_function_changed; ///< flag for changin transfer function
    kvs::TransferFunction m_transfer_function; ///< transfer function
    kvs::Texture2D m_preintegration_texture; ///< pre-integration texture
    kvs::Texture2D m_decomposition_texture; ///< texture for the tetrahedral decomposition
    kvs::Texture1D m_transfer_function_texture; ///< transfer function texture
    kvs::Texture1D m_T_texture; ///< T function for pre-integration
    kvs::Texture1D m_inv_T_texture; ///< inverse function of T for pre-integration
    kvs::VertexBufferObjectManager m_vbo_manager; ///< vertex buffer object manager
    float m_sampling_step; ///< sampling step
    float m_maxT; ///< maximum value of T

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
//    SSAODrawable m_drawable;
    Drawable m_drawable;

public:
    Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingStep( const float sampling_step ) { m_sampling_step = sampling_step; }
    void setTransferFunction( const kvs::TransferFunction& transfer_function )
    {
        m_transfer_function = transfer_function;
        m_transfer_function_changed = true;
    }

    float samplingStep() const { return m_sampling_step; }
    const kvs::TransferFunction& transferFunction() const { return m_transfer_function; }

    // void setSamplingSphereRadius( const kvs::Real32 radius ) { m_sampling_sphere_radius = radius; }
    // void setNumberOfSamplingPoints( const size_t nsamples ) { m_nsamples = nsamples; }
    // kvs::Real32 samplingSphereRadius() const { return m_sampling_sphere_radius; }
    // size_t numberOfSamplingPoints() const { return m_nsamples; }
    void setSamplingSphereRadius( const float radius ) { m_drawable.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_drawable.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_drawable.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_drawable.numberOfSamplingPoints(); }

private:
//    void create_shader_program();
    void create_buffer_object( const kvs::UnstructuredVolumeObject* volume );
    void create_preintegration_texture();
    void create_decomposition_texture();
//    void create_framebuffer( const size_t width, const size_t height );
//    void create_sampling_points();
//    void update_framebuffer( const size_t width, const size_t height );
    void render_geometry_pass( const kvs::UnstructuredVolumeObject* volume );
    void render_occlusion_pass();
};

} // end of namespace AmbientOcclusionRendering
