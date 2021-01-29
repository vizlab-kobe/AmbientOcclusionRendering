#pragma once
#include <kvs/DebugNew>
#include <kvs/Module>
#include <kvs/LineObject>
#include <kvs/LineRenderer>
#include <kvs/Shader>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObjectManager>
#include <kvs/Texture2D>
#include "StochasticRenderingEngine.h"
#include "StochasticRendererBase.h"
#include <kvs/TransferFunction>
#include <kvs/StylizedLineRenderer>


namespace local
{

class SSAOStochasticTubeRenderer : public local::StochasticRendererBase
{
    kvsModule( local::SSAOStochasticTubeRenderer, Renderer );
    kvsModuleBaseClass( local::StochasticRendererBase );

public:
    class Engine;

public:
    SSAOStochasticTubeRenderer();
    void setTransferFunction( const kvs::TransferFunction& tfunc );
    void setRadiusSize( const kvs::Real32 size );
    void setHaloSize( const kvs::Real32 size );
    const kvs::TransferFunction& transferFunction() const;
    kvs::Real32 radiusSize() const;
    kvs::Real32 haloSize() const;
    void setEdgeFactor( const float edge_factor );
};

class SSAOStochasticTubeRenderer::Engine : public local::StochasticRenderingEngine
{
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;

private:
    kvs::Real32 m_radius_size;
    kvs::Real32 m_halo_size;
    BufferObject m_buffer_object;

    bool m_tfunc_changed; ///< flag for changing transfer function
    kvs::TransferFunction m_tfunc; ///< transfer function
    kvs::Texture1D m_tfunc_texture; ///< transfer function texture
    kvs::ProgramObject m_geom_pass_shader;
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
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }

private:
    void create_buffer_object( const kvs::LineObject* line );
    void create_transfer_function_texture();
    void draw_buffer_object( const kvs::LineObject* line );
    void create_geometry_shader_program();
};

} // end of namespace local