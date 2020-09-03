/*****************************************************************************/
/**
 *  @file   StochasticRendererBase.h
 *  @author Jun Nishimura, Naohisa Sakamoto
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
#ifndef KVS__SSAO_STOCHASTIC_RENDERER_BASE_H_INCLUDE
#define KVS__SSAO_STOCHASTIC_RENDERER_BASE_H_INCLUDE

#include <kvs/RendererBase>
#include <kvs/Shader>
#include <kvs/Matrix44>
#include <kvs/Module>
#include <kvs/EnsembleAverageBuffer>
#include "SSAOStochasticRenderingEngine.h"
#include "SSAOFrameBuffer.h"


namespace local
{

class ObjectBase;
class Camera;
class Light;

/*===========================================================================*/
/**
 *  @brief  Stochastic renderer base class.
 */
/*===========================================================================*/
class SSAOStochasticRendererBase : public kvs::RendererBase
{
    kvsModule( local::SSAOStochasticRendererBase, Renderer );

    friend class StochasticRenderingCompositor;

private:

    size_t m_width;
    size_t m_height;
    size_t m_repetition_level; ///< repetition level
    size_t m_coarse_level; ///< repetition level for the coarse rendering (LOD)
	kvs::Real32 m_sampling_sphere_radius;
	size_t m_nsamples;
    bool m_enable_lod; ///< flag for LOD rendering
    bool m_enable_refinement; ///< flag for progressive refinement rendering
    kvs::Mat4 m_modelview; ///< modelview matrix used for LOD control
    kvs::Vec3 m_light_position; ///< light position used for LOD control
    kvs::EnsembleAverageBuffer m_ensemble_buffer; ///< ensemble averaging buffer
    kvs::Shader::ShadingModel* m_shader; ///< shading method
    local::SSAOStochasticRenderingEngine* m_engine; ///< rendering engine
    local::SSAOFrameBuffer m_ssao_framebuffer; ///< SSAO framebuffer 

public:

    SSAOStochasticRendererBase( local::SSAOStochasticRenderingEngine* engine );
    virtual ~SSAOStochasticRendererBase();

    virtual void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void release();
    size_t windowWidth() const { return m_width; }
    size_t windowHeight() const { return m_height; }
    size_t repetitionLevel() const { return m_repetition_level; }
    bool isEnabledLODControl() const { return m_enable_lod; }
    bool isEnabledRefinement() const { return m_enable_refinement; }
    void setRepetitionLevel( const size_t repetition_level ) { m_repetition_level = repetition_level; }
    void setEnabledLODControl( const bool enable ) { m_enable_lod = enable; }
    void setEnabledRefinement( const bool enable ) { m_enable_refinement = enable; }
	void setSamplingSphereRadius( const kvs::Real32 radius );
	void setNumberOfSamplingPoints( const size_t nsamples );
    void enableLODControl() { this->setEnabledLODControl( true ); }
    void enableRefinement() { this->setEnabledRefinement( true ); }
    void disableLODControl() { this->setEnabledLODControl( false ); }
    void disableRefinement() { this->setEnabledRefinement( false ); }
    const kvs::Shader::ShadingModel& shader() const { return *m_shader; }
    const local::SSAOStochasticRenderingEngine& engine() const { return *m_engine; }
    template <typename ShadingType>
    void setShader( const ShadingType shader );

public:

    kvs::Shader::ShadingModel& shader() { return *m_shader; }
    local::SSAOStochasticRenderingEngine& engine() { return *m_engine; }
};

template <typename ShadingType>
inline void SSAOStochasticRendererBase::setShader( const ShadingType shader )
{
    if ( m_shader ) { delete m_shader; }
    m_shader = new ShadingType( shader );
}

} // end of namespace kvs

#endif // KVS__STOCHASTIC_RENDERER_BASE_H_INCLUDE
