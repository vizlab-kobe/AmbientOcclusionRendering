#include "SSAOStochasticPolygonRenderer.h"
#include "SSAOPointSampling.h"
#include <cmath>
#include <kvs/OpenGL>
#include <kvs/PolygonObject>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Assert>
#include <kvs/Message>
#include <kvs/Xorshift128>


namespace
{

/*===========================================================================*/
/**
 *  @brief  Returns a random number as integer value.
 *  @return random number
 */
/*===========================================================================*/
int RandomNumber()
{
    const int C = 12347;
    static kvs::Xorshift128 R;
    return C * R.randInteger();
}

/*===========================================================================*/
/**
 *  @brief  Returns true if the polygon object has the connectivity.
 *  @param  polygon [in] pointer to the polygon object
 *  @return true if the polygon object has the connectivity
 */
/*===========================================================================*/
bool HasConnections( const kvs::PolygonObject* polygon )
{
    bool has_connection = polygon->numberOfConnections() > 0;

    // In the following cases, the connection stored in the polygon object will be ignored.
    if ( polygon->normalType() == kvs::PolygonObject::PolygonNormal ) { has_connection = false; }
    if ( polygon->colorType() == kvs::PolygonObject::PolygonColor ) { has_connection = false; }

    return has_connection;
}

/*===========================================================================*/
/**
 *  @brief  Returns number of vertices of the polygon object
 *  @param  polygon [in] pointer to the polygon object
 *  @return number of vertices
 */
/*===========================================================================*/
size_t NumberOfVertices( const kvs::PolygonObject* polygon )
{
    if ( polygon->connections().size() > 0 &&
         ( polygon->normalType() == kvs::PolygonObject::PolygonNormal ||
           polygon->colorType() == kvs::PolygonObject::PolygonColor ) )
    {
        const size_t nfaces = polygon->numberOfConnections();
        return nfaces * 3;
    }

    return polygon->numberOfVertices();
}

/*===========================================================================*/
/**
 *  @brief  Returns coordinate array.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::Real32> VertexCoords( const kvs::PolygonObject* polygon )
{
    if ( polygon->connections().size() > 0 &&
         ( polygon->normalType() == kvs::PolygonObject::PolygonNormal ||
           polygon->colorType() == kvs::PolygonObject::PolygonColor ) )
    {
        const size_t nfaces = polygon->numberOfConnections();
        const kvs::Real32* polygon_coords = polygon->coords().data();
        const kvs::UInt32* polygon_connections = polygon->connections().data();

        kvs::ValueArray<kvs::Real32> coords( nfaces * 9 );
        for ( size_t i = 0; i < nfaces; i++ )
        {
            const kvs::UInt32 id0 = polygon_connections[ i * 3 + 0 ];
            const kvs::UInt32 id1 = polygon_connections[ i * 3 + 1 ];
            const kvs::UInt32 id2 = polygon_connections[ i * 3 + 2 ];

            coords[ i * 9 + 0 ] = polygon_coords[ id0 * 3 + 0 ];
            coords[ i * 9 + 1 ] = polygon_coords[ id0 * 3 + 1 ];
            coords[ i * 9 + 2 ] = polygon_coords[ id0 * 3 + 2 ];

            coords[ i * 9 + 3 ] = polygon_coords[ id1 * 3 + 0 ];
            coords[ i * 9 + 4 ] = polygon_coords[ id1 * 3 + 1 ];
            coords[ i * 9 + 5 ] = polygon_coords[ id1 * 3 + 2 ];

            coords[ i * 9 + 6 ] = polygon_coords[ id2 * 3 + 0 ];
            coords[ i * 9 + 7 ] = polygon_coords[ id2 * 3 + 1 ];
            coords[ i * 9 + 8 ] = polygon_coords[ id2 * 3 + 2 ];
        }

        return coords;
    }

    return polygon->coords();
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
        if ( polygon->connections().size() > 0 &&
             polygon->colorType() == kvs::PolygonObject::PolygonColor )
        {
            const size_t nfaces = polygon->numberOfConnections();
            const kvs::Real32* polygon_normals = polygon->normals().data();
            const kvs::UInt32* polygon_connections = polygon->connections().data();
            normals.allocate( nfaces * 9 );
            for ( size_t i = 0; i < nfaces; i++ )
            {
                const kvs::UInt32 id0 = polygon_connections[ i * 3 + 0 ];
                const kvs::UInt32 id1 = polygon_connections[ i * 3 + 1 ];
                const kvs::UInt32 id2 = polygon_connections[ i * 3 + 2 ];

                normals[ i * 9 + 0 ] = polygon_normals[ id0 * 3 + 0 ];
                normals[ i * 9 + 1 ] = polygon_normals[ id0 * 3 + 1 ];
                normals[ i * 9 + 2 ] = polygon_normals[ id0 * 3 + 2 ];

                normals[ i * 9 + 3 ] = polygon_normals[ id1 * 3 + 0 ];
                normals[ i * 9 + 4 ] = polygon_normals[ id1 * 3 + 1 ];
                normals[ i * 9 + 5 ] = polygon_normals[ id1 * 3 + 2 ];

                normals[ i * 9 + 6 ] = polygon_normals[ id2 * 3 + 0 ];
                normals[ i * 9 + 7 ] = polygon_normals[ id2 * 3 + 1 ];
                normals[ i * 9 + 8 ] = polygon_normals[ id2 * 3 + 2 ];
            }
        }
        else
        {
            normals = polygon->normals();
        }
        break;
    }
    case kvs::PolygonObject::PolygonNormal:
    {
        const size_t nfaces = NumberOfVertices( polygon ) / 3;
        const kvs::Real32* polygon_normals = polygon->normals().data();
        normals.allocate( nfaces * 9 );
        for ( size_t i = 0; i < nfaces; i++ )
        {
            const kvs::Real32 nx = polygon_normals[ i * 3 + 0 ];
            const kvs::Real32 ny = polygon_normals[ i * 3 + 1 ];
            const kvs::Real32 nz = polygon_normals[ i * 3 + 2 ];

            normals[ i * 9 + 0 ] = nx;
            normals[ i * 9 + 1 ] = ny;
            normals[ i * 9 + 2 ] = nz;

            normals[ i * 9 + 3 ] = nx;
            normals[ i * 9 + 4 ] = ny;
            normals[ i * 9 + 5 ] = nz;

            normals[ i * 9 + 6 ] = nx;
            normals[ i * 9 + 7 ] = ny;
            normals[ i * 9 + 8 ] = nz;
        }
        break;
    }
    default: break;
    }

    return normals;
}

/*===========================================================================*/
/**
 *  @brief  Returns vertex-color array.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::UInt8> VertexColors( const kvs::PolygonObject* polygon )
{
    const bool is_single_color = polygon->colors().size() == 3;
    const bool is_single_alpha = polygon->opacities().size() == 1;

    if ( polygon->colors().size() == 0 )
    {
        return kvs::ValueArray<kvs::UInt8>();
    }

    kvs::ValueArray<kvs::UInt8> colors;
    switch ( polygon->colorType() )
    {
    case kvs::PolygonObject::VertexColor:
    {
        if ( polygon->connections().size() > 0 &&
             polygon->normalType() == kvs::PolygonObject::PolygonNormal )
        {
            const size_t nfaces = polygon->numberOfConnections();
            const kvs::UInt8* polygon_colors = polygon->colors().data();
            const kvs::UInt8* polygon_alphas = polygon->opacities().data();
            const kvs::UInt32* polygon_connections = polygon->connections().data();
            colors.allocate( nfaces * 12 );
            for ( size_t i = 0; i < nfaces; i++ )
            {
                const kvs::UInt32 id0 = polygon_connections[ i * 3 + 0 ];
                const kvs::UInt32 id1 = polygon_connections[ i * 3 + 1 ];
                const kvs::UInt32 id2 = polygon_connections[ i * 3 + 2 ];

                const size_t idr0 = ( is_single_color ) ? 0 : id0 * 3 + 0;
                const size_t idg0 = ( is_single_color ) ? 1 : id0 * 3 + 1;
                const size_t idb0 = ( is_single_color ) ? 2 : id0 * 3 + 2;
                const size_t ida0 = ( is_single_alpha ) ? 0 : id0;
                colors[ i * 12 +  0 ] = polygon_colors[ idr0 ];
                colors[ i * 12 +  1 ] = polygon_colors[ idg0 ];
                colors[ i * 12 +  2 ] = polygon_colors[ idb0 ];
                colors[ i * 12 +  3 ] = polygon_alphas[ ida0 ];

                const size_t idr1 = ( is_single_color ) ? 0 : id1 * 3 + 0;
                const size_t idg1 = ( is_single_color ) ? 1 : id1 * 3 + 1;
                const size_t idb1 = ( is_single_color ) ? 2 : id1 * 3 + 2;
                const size_t ida1 = ( is_single_alpha ) ? 0 : id1;
                colors[ i * 12 +  4 ] = polygon_colors[ idr1 ];
                colors[ i * 12 +  5 ] = polygon_colors[ idg1 ];
                colors[ i * 12 +  6 ] = polygon_colors[ idb1 ];
                colors[ i * 12 +  7 ] = polygon_alphas[ ida1 ];

                const size_t idr2 = ( is_single_color ) ? 0 : id2 * 3 + 0;
                const size_t idg2 = ( is_single_color ) ? 1 : id2 * 3 + 1;
                const size_t idb2 = ( is_single_color ) ? 2 : id2 * 3 + 2;
                const size_t ida2 = ( is_single_alpha ) ? 0 : id2;
                colors[ i * 12 +  8 ] = polygon_colors[ idr2 ];
                colors[ i * 12 +  9 ] = polygon_colors[ idg2 ];
                colors[ i * 12 + 10 ] = polygon_colors[ idb2 ];
                colors[ i * 12 + 11 ] = polygon_alphas[ ida2 ];
            }
        }
        else
        {
            const size_t nverts = polygon->numberOfVertices();
            colors.allocate( nverts * 4 );
            if ( is_single_color )
            {
                const kvs::RGBColor polygon_color = polygon->color();
                if ( is_single_alpha )
                {
                    const kvs::UInt8 polygon_alpha = polygon->opacity();
                    for ( size_t i = 0; i < nverts; i++ )
                    {
                        colors[ 4 * i + 0 ] = polygon_color.r();
                        colors[ 4 * i + 1 ] = polygon_color.g();
                        colors[ 4 * i + 2 ] = polygon_color.b();
                        colors[ 4 * i + 3 ] = polygon_alpha;
                    }
                }
                else
                {
                    const kvs::UInt8* polygon_alphas = polygon->opacities().data();
                    for ( size_t i = 0; i < nverts; i++ )
                    {
                        colors[ 4 * i + 0 ] = polygon_color.r();
                        colors[ 4 * i + 1 ] = polygon_color.g();
                        colors[ 4 * i + 2 ] = polygon_color.b();
                        colors[ 4 * i + 3 ] = polygon_alphas[ i ];
                    }
                }
            }
            else
            {
                const kvs::UInt8* polygon_colors = polygon->colors().data();
                if ( is_single_alpha )
                {
                    const kvs::UInt8 polygon_alpha = polygon->opacity();
                    for ( size_t i = 0; i < nverts; i++ )
                    {
                        colors[ 4 * i + 0 ] = polygon_colors[ 3 * i + 0 ];
                        colors[ 4 * i + 1 ] = polygon_colors[ 3 * i + 1 ];
                        colors[ 4 * i + 2 ] = polygon_colors[ 3 * i + 2 ];
                        colors[ 4 * i + 3 ] = polygon_alpha;
                    }
                }
                else
                {
                    const kvs::UInt8* polygon_alphas = polygon->opacities().data();
                    for ( size_t i = 0; i < nverts; i++ )
                    {
                        colors[ 4 * i + 0 ] = polygon_colors[ 3 * i + 0 ];
                        colors[ 4 * i + 1 ] = polygon_colors[ 3 * i + 1 ];
                        colors[ 4 * i + 2 ] = polygon_colors[ 3 * i + 2 ];
                        colors[ 4 * i + 3 ] = polygon_alphas[ i ];
                    }
                }
            }
        }
        break;
    }
    case kvs::PolygonObject::PolygonColor:
    {
        const size_t nfaces = NumberOfVertices( polygon ) / 3;
        const kvs::UInt8* polygon_colors = polygon->colors().data();
        const kvs::UInt8* polygon_alphas = polygon->opacities().data();
        colors.allocate( nfaces * 12 );
        for ( size_t i = 0; i < nfaces; i++ )
        {
            const kvs::UInt8 r = ( is_single_color ) ? polygon_colors[0] : polygon_colors[ i * 3 + 0 ];
            const kvs::UInt8 g = ( is_single_color ) ? polygon_colors[1] : polygon_colors[ i * 3 + 1 ];
            const kvs::UInt8 b = ( is_single_color ) ? polygon_colors[2] : polygon_colors[ i * 3 + 2 ];
            const kvs::UInt8 a = ( is_single_alpha ) ? polygon_alphas[0] : polygon_alphas[ i ];

            colors[ i * 12 +  0 ] = r;
            colors[ i * 12 +  1 ] = g;
            colors[ i * 12 +  2 ] = b;
            colors[ i * 12 +  3 ] = a;

            colors[ i * 12 +  4 ] = r;
            colors[ i * 12 +  5 ] = g;
            colors[ i * 12 +  6 ] = b;
            colors[ i * 12 +  7 ] = a;

            colors[ i * 12 +  8 ] = r;
            colors[ i * 12 +  9 ] = g;
            colors[ i * 12 + 10 ] = b;
            colors[ i * 12 + 11 ] = a;
        }
        break;
    }
    default: { break; }
    }

    return colors;
}

inline void Draw()
{
    kvs::OpenGL::WithPushedMatrix p1( GL_MODELVIEW );
    p1.loadIdentity();
    {
        kvs::OpenGL::WithPushedMatrix p2( GL_PROJECTION );
        p2.loadIdentity();
        {
            kvs::OpenGL::SetOrtho( 0, 1, 0, 1, -1, 1 );
            {
                kvs::OpenGL::Begin( GL_QUADS );
                kvs::OpenGL::Color( kvs::Vec4::All( 1.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 1 ), kvs::Vec2( 1, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 1 ), kvs::Vec2( 0, 1 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 0 ), kvs::Vec2( 0, 0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 0 ), kvs::Vec2( 1, 0 ) );
                kvs::OpenGL::End();
            }
        }
    }
}

}


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new SSAOStochasticPolygonRenderer class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::SSAOStochasticPolygonRenderer():
    StochasticRendererBase( new Engine() )
{
}

/*===========================================================================*/
/**
 *  @brief  Sets a polygon offset.
 *  @param  offset [in] offset value
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::setPolygonOffset( const float offset )
{
    static_cast<Engine&>( engine() ).setPolygonOffset( offset );
}

void SSAOStochasticPolygonRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

void SSAOStochasticPolygonRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

kvs::Real32 SSAOStochasticPolygonRenderer::samplingSphereRadius() const
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

size_t SSAOStochasticPolygonRenderer::numberOfSamplingPoints() const
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticPolygonRenderer::Engine::Engine():
    m_has_normal( false ),
    m_has_connection( false ),
    m_polygon_offset( 0.0f ),
    m_sampling_sphere_radius( 0.5f ),
    m_nsamples( 256 )
{
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::release()
{
    m_shader_geom_pass.release();
    m_shader_occl_pass.release();
    m_vbo_manager.release();
    m_framebuffer.release();
    m_color_texture.release();
    m_position_texture.release();
    m_normal_texture.release();
    m_depth_texture.release();
}

/*===========================================================================*/
/**
 *  @brief  Create shaders, VBO, and framebuffers.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::PolygonObject* polygon = kvs::PolygonObject::DownCast( object );
    m_has_normal = polygon->normals().size() > 0;
    m_has_connection = ::HasConnections( polygon );
    if ( !m_has_normal ) setEnabledShading( false );

    attachObject( object );
    createRandomTexture();
    this->create_shader_program();
    this->create_buffer_object( polygon );
    this->create_framebuffer( camera->windowWidth(), camera->windowHeight() );
    this->create_sampling_points();
}

/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    this->update_framebuffer( camera->windowWidth(), camera->windowHeight() );
}

/*===========================================================================*/
/**
 *  @brief  Set up.
 *  @param  polygon [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::setup(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 PM = kvs::OpenGL::ProjectionMatrix() * M;
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    m_shader_geom_pass.bind();
    m_shader_geom_pass.setUniform( "ModelViewMatrix", M );
    m_shader_geom_pass.setUniform( "ModelViewProjectionMatrix", PM );
    m_shader_geom_pass.setUniform( "NormalMatrix", N );
    m_shader_geom_pass.setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
    m_shader_geom_pass.setUniform( "random_texture", 0 );
    m_shader_geom_pass.setUniform( "polygon_offset", m_polygon_offset );
    m_shader_geom_pass.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Draw an ensemble.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    this->render_geometry_pass( kvs::PolygonObject::DownCast( object ) );
    this->render_occlusion_pass();
}

/*===========================================================================*/
/**
 *  @brief  Creates shader program.
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create_shader_program()
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( "SSAO_stochastic_polygon_geom_pass.vert" );
        kvs::ShaderSource frag( "SSAO_stochastic_polygon_geom_pass.frag" );
        m_shader_geom_pass.build( vert, frag );
    }

    // Build SSAO shader for occlusion-pass (2nd pass).
    {
        kvs::ShaderSource vert( "SSAO_occl_pass.vert" );
        kvs::ShaderSource frag( "SSAO_occl_pass.frag" );
        if ( isEnabledShading() )
        {
            switch ( shader().type() )
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
        m_shader_occl_pass.setUniform( "shading.Ka", shader().Ka );
        m_shader_occl_pass.setUniform( "shading.Kd", shader().Kd );
        m_shader_occl_pass.setUniform( "shading.Ks", shader().Ks );
        m_shader_occl_pass.setUniform( "shading.S",  shader().S );
        m_shader_occl_pass.unbind();
    }
}

/*===========================================================================*/
/**
 *  @brief  Create buffer objects.
 *  @param  polygon [in] pointer to the polygon object
 */
/*===========================================================================*/
void SSAOStochasticPolygonRenderer::Engine::create_buffer_object( const kvs::PolygonObject* polygon )
{
    if ( polygon->polygonType() != kvs::PolygonObject::Triangle )
    {
        kvsMessageError( "Not supported polygon type." );
        return;
    }

    const size_t nvertices = ::NumberOfVertices( polygon );
    kvs::ValueArray<kvs::UInt16> indices( nvertices * 2 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        const unsigned int count = i * 12347;
        indices[ 2 * i + 0 ] = static_cast<kvs::UInt16>( ( count ) % randomTextureSize() );
        indices[ 2 * i + 1 ] = static_cast<kvs::UInt16>( ( count / randomTextureSize() ) % randomTextureSize() );
    }
    kvs::ValueArray<kvs::Real32> coords = ::VertexCoords( polygon );
    kvs::ValueArray<kvs::UInt8> colors = ::VertexColors( polygon );
    kvs::ValueArray<kvs::Real32> normals = ::VertexNormals( polygon );

    m_vbo_manager.setVertexAttribArray( indices, m_shader_geom_pass.attributeLocation( "random_index" ), 2 );
    m_vbo_manager.setVertexArray( coords, 3 );
    m_vbo_manager.setColorArray( colors, 4 );
    if ( normals.size() > 0 ) { m_vbo_manager.setNormalArray( normals ); }
    if ( m_has_connection ) { m_vbo_manager.setIndexArray( polygon->connections() ); }
    m_vbo_manager.create();
}

void SSAOStochasticPolygonRenderer::Engine::create_framebuffer( const size_t width, const size_t height )
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

void SSAOStochasticPolygonRenderer::Engine::create_sampling_points()
{
    const size_t nsamples = m_nsamples;
    const float radius = m_sampling_sphere_radius;
    const size_t dim = 3;
    const kvs::ValueArray<GLfloat> sampling_points = AmbientOcclusionRendering::SSAOPointSampling( radius, nsamples );
    m_shader_occl_pass.bind();
    m_shader_occl_pass.setUniform( "sampling_points", sampling_points, dim );
    m_shader_occl_pass.unbind();
}

void SSAOStochasticPolygonRenderer::Engine::update_framebuffer( const size_t width, const size_t height )
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

void SSAOStochasticPolygonRenderer::Engine::render_geometry_pass( const kvs::PolygonObject* polygon )
{
    kvs::FrameBufferObject::GuardedBinder bind0( m_framebuffer );

    // Initialize FBO.
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Enable MRT rendering.
    const GLenum buffers[3] = {
        GL_COLOR_ATTACHMENT0_EXT,
        GL_COLOR_ATTACHMENT1_EXT,
        GL_COLOR_ATTACHMENT2_EXT };
    kvs::OpenGL::SetDrawBuffers( 3, buffers );

    kvs::VertexBufferObjectManager::Binder bind1( m_vbo_manager );
    kvs::ProgramObject::Binder bind2( m_shader_geom_pass );
    kvs::Texture::Binder bind3( randomTexture() );
    {
        kvs::OpenGL::WithEnabled d( GL_DEPTH_TEST );
        kvs::OpenGL::SetPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        const size_t size = randomTextureSize();
        const int count = repetitionCount() * ::RandomNumber();
        const float offset_x = static_cast<float>( ( count ) % size );
        const float offset_y = static_cast<float>( ( count / size ) % size );
        const kvs::Vec2 random_offset( offset_x, offset_y );
        m_shader_geom_pass.setUniform( "random_offset", random_offset );

        const size_t nconnections = polygon->numberOfConnections();
        const size_t nvertices = ::NumberOfVertices( polygon );
        const size_t npolygons = nconnections == 0 ? nvertices / 3 : nconnections;

        // Draw triangles.
        if ( m_has_connection ) { m_vbo_manager.drawElements( GL_TRIANGLES, 3 * npolygons ); }
        else { m_vbo_manager.drawArrays( GL_TRIANGLES, 0, 3 * npolygons ); }
    }
}

void SSAOStochasticPolygonRenderer::Engine::render_occlusion_pass()
{
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

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );
    ::Draw();
}

} // end of namespace AmbientOcclusionRendering
