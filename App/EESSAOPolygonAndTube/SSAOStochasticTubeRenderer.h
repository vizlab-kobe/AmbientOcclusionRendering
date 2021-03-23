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

class SSAOStochasticTubeRenderer::Engine : public kvs::StochasticRenderingEngine
{
public:
    using BaseClass = kvs::StochasticRenderingEngine;
    using BufferObject = kvs::StylizedLineRenderer::BufferObject;

    class RenderPass : public kvs::StylizedLineRenderer::RenderPass
    {
    private:
        using BaseRenderPass = kvs::StylizedLineRenderer::RenderPass;
        using Parent = BaseClass;
        const Parent* m_parent; ///< reference to the engine
    public:
        RenderPass( BufferObject& buffer_object, Parent* parent );
        void create( const kvs::Shader::ShadingModel& model, const bool enable );
        void setup( const kvs::Shader::ShadingModel& model );
    };

private:
    float m_edge_factor = 0.0f;;
    BufferObject m_buffer_object{};
    RenderPass m_render_pass{ m_buffer_object, this };

    bool m_tfunc_changed = true; ///< flag for changing transfer function
    kvs::TransferFunction m_tfunc{}; ///< transfer function
    kvs::Texture1D m_tfunc_texture{}; ///< transfer function texture

public:
    Engine() = default;
    void release() { m_render_pass.release(); m_buffer_object.release(); m_tfunc_changed = true; }
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

public:
    void setTransferFunction( const kvs::TransferFunction& tfunc ) { m_tfunc = tfunc; m_tfunc_changed = true; }
    void setRadiusSize( const kvs::Real32 size ) { m_render_pass.setRadiusSize( size ); }
    void setHaloSize( const kvs::Real32 size ) { m_render_pass.setHaloSize( size ); }
    const kvs::TransferFunction& transferFunction() const { return m_tfunc; }
    kvs::Real32 radiusSize() const { return m_render_pass.radiusSize(); }
    kvs::Real32 haloSize() const { return m_render_pass.haloSize(); }
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }

private:
    void create_transfer_function_texture();
    void update_transfer_function_texture();

    void create_buffer_object( const kvs::LineObject* line );
    void update_buffer_object( const kvs::LineObject* line );
    void draw_buffer_object( const kvs::LineObject* line );
};

} // end of namespace local
