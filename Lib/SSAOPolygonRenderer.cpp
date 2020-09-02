/*****************************************************************************/
/**
 *  @file   SSAOPolygonRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOPolygonRenderer.h"
//#include "SSAOPointSampling.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
//#include <kvs/ShaderSource>
//#include <kvs/VertexShader>
//#include <kvs/FragmentShader>
//#include <kvs/String>


namespace
{

/*===========================================================================*/
/**
 *  @brief  Returns vertex-color array.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::UInt8> VertexColors( const kvs::PolygonObject* polygon )
{
    const size_t nvertices = polygon->numberOfVertices();
    const bool is_single_color = polygon->colors().size() == 3;
    const bool is_single_alpha = polygon->opacities().size() == 1;
    const kvs::UInt8* pcolors = polygon->colors().data();
    const kvs::UInt8* palphas = polygon->opacities().data();

    kvs::ValueArray<kvs::UInt8> colors( nvertices * 4 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        colors[ 4 * i + 0 ] = is_single_color ? pcolors[0] : pcolors[ 3 * i + 0 ];
        colors[ 4 * i + 1 ] = is_single_color ? pcolors[1] : pcolors[ 3 * i + 1 ];
        colors[ 4 * i + 2 ] = is_single_color ? pcolors[2] : pcolors[ 3 * i + 2 ];
        colors[ 4 * i + 3 ] = is_single_alpha ? palphas[0] : palphas[i];
    }

    return colors;
}

/*===========================================================================*/
/**
 *  @brief  Returns vertex-normal array.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::Real32> VertexNormals( const kvs::PolygonObject* polygon )
{
    if ( polygon->normals().size() == 0 )
    {
        return kvs::ValueArray<kvs::Real32>();
    }

    kvs::ValueArray<kvs::Real32> normals;
    switch ( polygon->normalType() )
    {
    case kvs::PolygonObject::VertexNormal:
    {
        normals = polygon->normals();
        break;
    }
    case kvs::PolygonObject::PolygonNormal:
    {
        // Same normal vectors are assigned for each vertex of the polygon.
        const size_t npolygons = polygon->normals().size() / 3;
        const size_t nnormals = npolygons * 3;
        normals.allocate( nnormals * 3 );
        kvs::Real32* pnormals = normals.data();
        for ( size_t i = 0; i < npolygons; i++ )
        {
            const kvs::Vec3 n = polygon->normal(i);
            for ( size_t j = 0; j < 3; j++ )
            {
                *(pnormals++) = n.x();
                *(pnormals++) = n.y();
                *(pnormals++) = n.z();
            }
        }
        break;
    }
    default: break;
    }

    return normals;
}

/*
inline void Draw()
{
    kvs::OpenGL::WithPushedMatrix p1( GL_MODELVIEW );
    p1.loadIdentity();
    {
        kvs::OpenGL::WithPushedMatrix p2( GL_PROJECTION );
        p2.loadIdentity();
        {
            kvs::OpenGL::SetOrtho( 0, 1, 0, 1, -1, 1 );
            kvs::OpenGL::WithDisabled d1( GL_DEPTH_TEST );
            kvs::OpenGL::WithDisabled d2( GL_LIGHTING );
            kvs::OpenGL::WithEnabled e1( GL_TEXTURE_2D );
            {
                kvs::OpenGL::Begin( GL_QUADS );
                kvs::OpenGL::Color( kvs::Vec4::Constant( 1.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 1 ), kvs::Vec2( 1, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 1 ), kvs::Vec2( 0, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 0 ), kvs::Vec2( 0, 0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 0 ), kvs::Vec2( 1, 0 ) );
                kvs::OpenGL::End();
            }
        }
    }
}
*/

} // end of namespace


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOPolygonRenderer class.
 */
/*===========================================================================*/
SSAOPolygonRenderer::SSAOPolygonRenderer():
    m_window_width( 0 ),
    m_window_height( 0 ),
    m_object( NULL ),
    m_has_normal( false ),
    m_has_connection( false ),
    m_shader( NULL )
//    m_sampling_sphere_radius( 0.5f ),
//    m_nsamples( 256 )
{
    this->setShader( kvs::Shader::Lambert() );
    m_drawable.setGeometryPassShaderFiles( "SSAO_polygon_geom_pass.vert", "SSAO_polygon_geom_pass.frag" );
    m_drawable.setOcclusionPassShaderFiles( "SSAO_occl_pass.vert", "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Destroys the SSAOPolygonRenderer class.
 */
/*===========================================================================*/
SSAOPolygonRenderer::~SSAOPolygonRenderer()
{
    if ( m_shader ) { delete m_shader; }
}

/*===========================================================================*/
/**
 *  @brief  Executes rendering process.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOPolygonRenderer::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::PolygonObject* polygon = kvs::PolygonObject::DownCast( object );
    m_has_normal = polygon->normals().size() > 0;
    m_has_connection = polygon->numberOfConnections() > 0;
    if ( !m_has_normal ) setEnabledShading( false );

    BaseClass::startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );
    kvs::OpenGL::Enable( GL_DEPTH_TEST );

    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( width * dpr );
    const size_t framebuffer_height = static_cast<size_t>( height * dpr );

    const bool window_created = m_window_width == 0 && m_window_height == 0;
    if ( window_created )
    {
        m_window_width = width;
        m_window_height = height;
        m_object = object;
        m_drawable.createShaderProgram( *m_shader, isEnabledShading() );
        m_drawable.createFramebuffer( framebuffer_width, framebuffer_height );
        this->create_buffer_object( polygon );
//        this->create_shader_program();
//        this->create_buffer_object( polygon );
//        this->create_framebuffer( framebuffer_width, framebuffer_height );
//        this->create_sampling_points();
    }

    const bool window_resized = m_window_width != width || m_window_height != height;
    if ( window_resized )
    {
        m_window_width = width;
        m_window_height = height;
        m_drawable.updateFramebuffer( framebuffer_width, framebuffer_height );
//        this->update_framebuffer( framebuffer_width, framebuffer_height );
    }

    const bool object_changed = m_object != object;
    if ( object_changed )
    {
        m_object = object;
//        m_shader_geom_pass.release();
//        m_shader_occl_pass.release();
        m_vbo_manager.release();
//        m_framebuffer.release();
//        m_color_texture.release();
//        m_position_texture.release();
//        m_normal_texture.release();
//        m_depth_texture.release();
//        this->create_shader_program();
        m_drawable.updateShaderProgram( *m_shader, isEnabledShading() );
        m_drawable.updateFramebuffer( framebuffer_width, framebuffer_height );
        this->create_buffer_object( polygon );
    }

    // Ambient occlusion.
    this->render_geometry_pass( polygon );
    this->render_occlusion_pass();

    BaseClass::stopTimer();
}

/*===========================================================================*/
/**
 *  @brief  Creates shader program.
 */
/*===========================================================================*/
/*
void SSAOPolygonRenderer::create_shader_program()
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( "SSAO_polygon_geom_pass.vert" );
        kvs::ShaderSource frag( "SSAO_polygon_geom_pass.frag" );

        m_shader_geom_pass.build( vert, frag );
    }

    // Build SSAO shader for occlusion-pass (2nd pass).
    {
        kvs::ShaderSource vert( "SSAO_occl_pass.vert" );
        kvs::ShaderSource frag( "SSAO_occl_pass.frag" );

        if ( isEnabledShading() )
        {
            switch ( m_shader->type() )
            {
            case kvs::Shader::LambertShading: frag.define("ENABLE_LAMBERT_SHADING"); break;
            case kvs::Shader::PhongShading: frag.define("ENABLE_PHONG_SHADING"); break;
            case kvs::Shader::BlinnPhongShading: frag.define("ENABLE_BLINN_PHONG_SHADING"); break;
            default: break; // NO SHADING
            }

            if ( kvs::OpenGL::Boolean( GL_LIGHT_MODEL_TWO_SIDE ) == GL_TRUE )
            {
                frag.define("ENABLE_TWO_SIDE_LIGHTING");
            }

            frag.define( "NUMBER_OF_SAMPLING_POINTS " + kvs::String::ToString( m_nsamples ) );
        }

        m_shader_occl_pass.build( vert, frag );
        m_shader_occl_pass.bind();
        m_shader_occl_pass.setUniform( "shading.Ka", m_shader->Ka );
        m_shader_occl_pass.setUniform( "shading.Kd", m_shader->Kd );
        m_shader_occl_pass.setUniform( "shading.Ks", m_shader->Ks );
        m_shader_occl_pass.setUniform( "shading.S",  m_shader->S );
        m_shader_occl_pass.unbind();
    }
}
*/

/*===========================================================================*/
/**
 *  @brief  Creates buffer object.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOPolygonRenderer::create_buffer_object( const kvs::PolygonObject* polygon )
{
    if ( polygon->polygonType() != kvs::PolygonObject::Triangle )
    {
        kvsMessageError() << "Not supported polygon type." << std::endl;;
        return;
    }

    if ( polygon->colors().size() != 3 && polygon->colorType() == kvs::PolygonObject::PolygonColor )
    {
        kvsMessageError() << "Not supported polygon color type." << std::endl;
        return;
    }

    kvs::ValueArray<kvs::Real32> coords = polygon->coords();
    kvs::ValueArray<kvs::UInt8> colors = ::VertexColors( polygon );
    kvs::ValueArray<kvs::Real32> normals = ::VertexNormals( polygon );

    m_vbo_manager.setVertexArray( coords, 3 );
    m_vbo_manager.setColorArray( colors, 3 );
    if ( normals.size() > 0 ) { m_vbo_manager.setNormalArray( normals ); }
    if ( m_has_connection ) { m_vbo_manager.setIndexArray( polygon->connections() ); }
    m_vbo_manager.create();
}

/*
void SSAOPolygonRenderer::create_framebuffer( const size_t width, const size_t height )
{
    m_color_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_color_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_color_texture.setMagFilter( GL_LINEAR );
    m_color_texture.setMinFilter( GL_LINEAR );
    m_color_texture.setPixelFormat( GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE );
    m_color_texture.create( width, height );

    m_position_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_position_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_position_texture.setMagFilter( GL_LINEAR );
    m_position_texture.setMinFilter( GL_LINEAR );
    m_position_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_position_texture.create( width, height );

    m_normal_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_normal_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_normal_texture.setMagFilter( GL_LINEAR );
    m_normal_texture.setMinFilter( GL_LINEAR );
    m_normal_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_normal_texture.create( width, height );

    m_depth_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_depth_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_depth_texture.setMagFilter( GL_LINEAR );
    m_depth_texture.setMinFilter( GL_LINEAR );
    m_depth_texture.setPixelFormat( GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT  );
    m_depth_texture.create( width, height );

    m_framebuffer.create();
    m_framebuffer.attachColorTexture( m_color_texture, 0 );
    m_framebuffer.attachColorTexture( m_position_texture, 1 );
    m_framebuffer.attachColorTexture( m_normal_texture, 2 );
    m_framebuffer.attachDepthTexture( m_depth_texture );
}
*/

/*
void SSAOPolygonRenderer::create_sampling_points()
{
    const size_t nsamples = m_nsamples;
    const float radius = m_sampling_sphere_radius;
    const size_t dim = 3;
    const kvs::ValueArray<GLfloat> sampling_points = AmbientOcclusionRendering::SSAOPointSampling( radius, nsamples );
    m_shader_occl_pass.bind();
    m_shader_occl_pass.setUniform( "sampling_points", sampling_points, dim );
    m_shader_occl_pass.unbind();
}
*/

/*
void SSAOPolygonRenderer::update_framebuffer( const size_t width, const size_t height )
{
    m_color_texture.release();
    m_color_texture.create( width, height );

    m_position_texture.release();
    m_position_texture.create( width, height );

    m_normal_texture.release();
    m_normal_texture.create( width, height );

    m_depth_texture.release();
    m_depth_texture.create( width, height );

    m_framebuffer.attachColorTexture( m_color_texture, 0 );
    m_framebuffer.attachColorTexture( m_position_texture, 1 );
    m_framebuffer.attachColorTexture( m_normal_texture, 2 );
    m_framebuffer.attachDepthTexture( m_depth_texture );
}
*/

void SSAOPolygonRenderer::render_geometry_pass( const kvs::PolygonObject* polygon )
{
//    kvs::FrameBufferObject::Binder bind0( m_framebuffer );
    kvs::FrameBufferObject::GuardedBinder bind0( m_drawable.framebuffer() );

    // Initialize FBO.
    kvs::OpenGL::SetClearColor( kvs::Vec4::Constant( 0.0f ) );
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Enable MRT rendering.
    const GLenum buffers[3] = {
        GL_COLOR_ATTACHMENT0_EXT,
        GL_COLOR_ATTACHMENT1_EXT,
        GL_COLOR_ATTACHMENT2_EXT };
    kvs::OpenGL::SetDrawBuffers( 3, buffers );

    kvs::VertexBufferObjectManager::Binder bind1( m_vbo_manager );
//    kvs::ProgramObject::Binder bind2( m_shader_geom_pass );
    kvs::ProgramObject::Binder bind2( m_drawable.geometryPassShader() );
    {
        kvs::OpenGL::SetPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        const size_t nconnections = polygon->numberOfConnections();
        const size_t nvertices = polygon->numberOfVertices();
        const size_t npolygons = nconnections == 0 ? nvertices / 3 : nconnections;

        // Draw triangles.
        if ( m_has_connection )
        {
            m_vbo_manager.drawElements( GL_TRIANGLES, 3 * npolygons );
        }
        else
        {
            m_vbo_manager.drawArrays( GL_TRIANGLES, 0, 3 * npolygons );
        }
    }
}

void SSAOPolygonRenderer::render_occlusion_pass()
{
    /*
    kvs::ProgramObject::Binder bind1( m_shader_occl_pass );
    kvs::Texture::Binder unit0( m_color_texture, 0 );
    kvs::Texture::Binder unit1( m_position_texture, 1 );
    kvs::Texture::Binder unit2( m_normal_texture, 2 );
    kvs::Texture::Binder unit3( m_depth_texture, 3 );
    m_shader_occl_pass.setUniform( "color_texture", 0 );
    m_shader_occl_pass.setUniform( "position_texture", 1 );
    m_shader_occl_pass.setUniform( "normal_texture", 2 );
    m_shader_occl_pass.setUniform( "depth_texture", 3 );
    m_shader_occl_pass.setUniform( "ProjectionMatrix", kvs::OpenGL::ProjectionMatrix() );
    ::Draw();
    */
    m_drawable.renderOcclusionPass();
}

} // end of namespace AmbientOcclusionRendering
