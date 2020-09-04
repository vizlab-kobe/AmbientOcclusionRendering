/*****************************************************************************/
/**
 *  @file   SSAOPolygonRenderer.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/DebugNew>
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include "SSAODrawable.h"


namespace AmbientOcclusionRendering
{

class SSAOPolygonRenderer : public kvs::glsl::PolygonRenderer
{
    kvsModule( AmbientOcclusionRendering::SSAOPolygonRenderer, Renderer );
    kvsModuleBaseClass( kvs::glsl::PolygonRenderer );

private:
    SSAODrawable m_drawable;

public:
    SSAOPolygonRenderer();
    virtual ~SSAOPolygonRenderer() {}

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    void setSamplingSphereRadius( const float radius ) { m_drawable.setSamplingSphereRadius( radius ); }
    void setNumberOfSamplingPoints( const size_t nsamples ) { m_drawable.setNumberOfSamplingPoints( nsamples ); }
    kvs::Real32 samplingSphereRadius() const { return m_drawable.samplingSphereRadius(); }
    size_t numberOfSamplingPoints() const { return m_drawable.numberOfSamplingPoints(); }

private:
    void createShaderProgram();
    void updateShaderProgram();

    void createFramebuffer( const size_t width, const size_t height );
    void updateFramebuffer( const size_t width, const size_t height );
};

} // end of namespace AmbientOcclusionRendering
