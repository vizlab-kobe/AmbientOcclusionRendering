/*****************************************************************************/
/**
 *  @file   StochasticUniformGridRenderer.h
 *  @author Naoya Maeda, Naohisa Sakamoto
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
#include <AmbientOcclusionRendering/Lib/AmbientOcclusionBuffer.h>


//namespace AmbientOcclusionRendering
namespace local
{

/*===========================================================================*/
/**
 *  @brief  Stochastic uniform grid renderer class.
 */
/*===========================================================================*/
class SSAOStochasticUniformGridRenderer : public kvs::StochasticRendererBase
{
//    kvsModule( AmbientOcclusionRendering::SSAOStochasticUniformGridRenderer, Renderer );
    kvsModule( local::SSAOStochasticUniformGridRenderer, Renderer );
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
    void setEdgeFactor( const float edge_factor );
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
    kvs::ProgramObject m_bounding_cube_shader; ///< bounding cube shader
    float m_edge_factor;

//    AmbientOcclusionBuffer m_ao_buffer;
    AmbientOcclusionRendering::AmbientOcclusionBuffer m_ao_buffer;

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

    void setSamplingSphereRadius( const float radius ) { m_ao_buffer.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_ao_buffer.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_ao_buffer.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_ao_buffer.numberOfSamplingPoints(); }
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }

private:
    void create_shader_program( const kvs::StructuredVolumeObject* volume );
    void create_volume_texture( const kvs::StructuredVolumeObject* volume );
    void create_transfer_function_texture();
    void create_bounding_cube_buffer( const kvs::StructuredVolumeObject* volume );
    void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );
    void draw_bounding_cube_buffer();
    void draw_quad();
    void draw_buffer_object( const kvs::StructuredVolumeObject* volume );
};

//} // end of namespace AmbientOcclusionRendering
} // end of namespace local