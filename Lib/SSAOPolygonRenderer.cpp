/*****************************************************************************/
/**
 *  @file   SSAOPolygonRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOPolygonRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>


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
    }

    const bool window_resized = m_window_width != width || m_window_height != height;
    if ( window_resized )
    {
        m_window_width = width;
        m_window_height = height;
        m_drawable.updateFramebuffer( framebuffer_width, framebuffer_height );
    }

    const bool object_changed = m_object != object;
    if ( object_changed )
    {
        m_object = object;
        m_vbo_manager.release();
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

void SSAOPolygonRenderer::render_geometry_pass( const kvs::PolygonObject* polygon )
{
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
    m_drawable.renderOcclusionPass();
}

} // end of namespace AmbientOcclusionRendering
