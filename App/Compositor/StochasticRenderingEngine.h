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
    
public:
    // Compositor engine.
};

} // end of namespace local
