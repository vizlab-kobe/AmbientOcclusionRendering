/*****************************************************************************/
/**
 *  @file   SSAORenderer.h
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
/*****************************************************************************/
#pragma once
#include <kvs/OpenGL>
#include <kvs/RendererBase>
#include <kvs/ClassName>
#include <kvs/Module>
#include <kvs/PolygonObject>
#include <kvs/Shader>
#include <kvs/Texture2D>
#include <kvs/FrameBufferObject>
#include <kvs/VertexBufferObject>
#include <kvs/IndexBufferObject>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/ProgramObject>


namespace kvs
{

namespace glew
{

class SSAORenderer : public kvs::RendererBase
{
    // Class name.
    kvsClassName( kvs::glew::SSAORenderer );

    // Module information.
    kvsModuleCategory( Renderer );
    kvsModuleBaseClass( kvs::RendererBase );

protected:

    typedef GLfloat  CoordType;
    typedef GLubyte  ColorType;
    typedef GLbyte   NormalType;
    typedef GLuint   ConnectType;

    // Reference only.
    const kvs::PolygonObject* m_ref_polygon;

    bool    m_is_debug_draw;

    size_t  m_width;
    size_t  m_height;

    kvs::VertexBufferObject   m_vbo;
    kvs::IndexBufferObject    m_ibo;
    kvs::ProgramObject        m_object_shader;
    kvs::ProgramObject        m_ssao_shader;
    kvs::Shader::shader_type*       m_shader;

    // Offset for VBO drawing.
    size_t  m_off_coord;
    size_t  m_off_color;
    size_t  m_off_normal;

    kvs::FrameBufferObject m_framebuffer;
    kvs::Texture2D m_color_texture;
    kvs::Texture2D m_position_texture;
    kvs::Texture2D m_normal_texture;
    kvs::Texture2D m_depth_texture;

public:
    SSAORenderer( void );
    virtual ~SSAORenderer( void );

public:

    void initialize( void );

    void attachPolygonObject( const kvs::PolygonObject* polygon );

    template <typename ShadingType>
    void setShader( const ShadingType shader );

    void enableDebugDraw( void );

    void disableDebugDraw( void );

public:

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

protected:

    void create_image( kvs::PolygonObject* polygon, kvs::Camera* camera, kvs::Light* light );

    void download_buffer_object( void );

    void draw_buffer_object( void );

    void initialize_shaders( void );

    void initialize_framebuffer( void );

    void create_texture(
        kvs::Texture2D& texture,
        kvs::FrameBufferObject& framebuffer,
        GLint format,
        GLenum attachment );

    void create_shaders(
        kvs::ProgramObject& program_object,
        const kvs::ShaderSource& vertex_source,
        const kvs::ShaderSource& fragment_source );

    void setup_shader_parameter( const GLfloat* projection_matrix );

protected:

    virtual void initialize_projection( void );

    virtual void initialize_modelview( void );

};

template <typename ShadingType>
inline void SSAORenderer::setShader( const ShadingType shader )
{
    if ( m_shader )
    {
        delete m_shader;
        m_shader = NULL;
    }

    m_shader = new ShadingType( shader );
    if ( !m_shader )
    {
        kvsMessageError( "Cannot create a specified shader." );
    }
};

}

}
