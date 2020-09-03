#pragma once
#include <kvs/DebugNew>
#include <kvs/Module>
#include <kvs/LineObject>
#include <kvs/LineRenderer>
#include <kvs/Shader>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/Texture2D>
#include "SSAOStochasticRenderingEngine.h"
#include "SSAOStochasticRendererBase.h"
#include <kvs/TransferFunction>


namespace local
{

class SSAOStochasticTubeRenderer : public local::SSAOStochasticRendererBase
{
    kvsModule( local::SSAOStochasticTubeRenderer, Renderer );
    kvsModuleBaseClass( local::SSAOStochasticRendererBase );

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
};

class SSAOStochasticTubeRenderer::Engine : public local::SSAOStochasticRenderingEngine
{
private:
    kvs::ValueArray<GLint> m_first_array; ///< array of starting indices for the polyline
    kvs::ValueArray<GLsizei> m_count_array; ///< array of the number of indices for the polyline
    kvs::ProgramObject m_shader_geom_pass; ///< shader program for geometry-pass (1st pass)

    // Variables for tube rendering
    kvs::Real32 m_radius_size;
    kvs::Real32 m_halo_size;
    kvs::Texture2D m_shape_texture;
    kvs::Texture2D m_diffuse_texture;
    kvs::VertexBufferObjectManager m_vbo_manager; ///< vertex buffer object manager

    bool m_tfunc_changed; ///< flag for changing transfer function
    kvs::TransferFunction m_tfunc; ///< transfer function
    kvs::Texture1D m_tfunc_texture; ///< transfer function texture

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

private:
    void create_shader_program();
    void create_buffer_object( const kvs::LineObject* line );
    void create_shape_texture();
    void create_diffuse_texture();
    void create_transfer_function_texture();
};

} // end of namespace local
