/*****************************************************************************/
/**
 *  @file   StochasticRenderingEngine.h
 *  @author Jun Nishimura, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/Shader>
#include <kvs/Texture2D>
#include <kvs/StochasticRenderingEngine>
#include <kvs/ProgramObject>
#include "AmbientOcclusionBuffer.h"


namespace local
{

class ObjectBase;
class Camera;
class Light;

/*===========================================================================*/
/**
 *  @brief  StochasticRenderingEngine class.
 */
/*===========================================================================*/
class StochasticRenderingEngine : public kvs::StochasticRenderingEngine
{
    friend class StochasticRendererBase;
    friend class StochasticRenderingCompositor;
    /*DEPRECATED*/ friend class StochasticMultipleTetrahedraCompositor;

public:
    kvs::ProgramObject m_geom_pass_shader;
    
public:
    // Compositor engine.
    virtual void create_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light ) = 0;
    virtual void update_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light ) = 0;
    virtual void setup_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light ) = 0;
    virtual void draw_c( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light ) = 0;

    kvs::ProgramObject& geometryPassShader() { return m_geom_pass_shader; }
};

} // end of namespace local
