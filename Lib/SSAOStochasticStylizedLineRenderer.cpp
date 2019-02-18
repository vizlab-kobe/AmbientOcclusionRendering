#include "SSAOStochasticStylizedLineRenderer.h"
#include "SSAOPointSampling.h"
#include <kvs/OpenGL>
#include <kvs/ProgramObject>
#include <kvs/ShaderSource>
#include <kvs/VertexShader>
#include <kvs/FragmentShader>
#include <kvs/Xorshift128>
#include <kvs/String>


namespace
{

/*===========================================================================*/
/**
 *  @brief  Returns a random number as integer value.
 *  @return random number
 */
/*===========================================================================*/
inline int RandomNumber()
{
    const int C = 12347;
    static kvs::Xorshift128 R;
    return C * R.randInteger();
}

inline kvs::ValueArray<kvs::UInt16> RandomIndices( const kvs::LineObject* line, const size_t tex_size )
{
    const size_t nvertices = line->numberOfVertices() * 2;
    kvs::ValueArray<kvs::UInt16> indices( nvertices * 2 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        const unsigned int count = i * 12347;
        indices[ 2 * i + 0 ] = static_cast<kvs::UInt16>( ( count ) % tex_size );
        indices[ 2 * i + 1 ] = static_cast<kvs::UInt16>( ( count / tex_size ) % tex_size );
    }
    return indices;
}

inline kvs::ValueArray<kvs::Real32> QuadVertexCoords( const kvs::LineObject* line )
{
    const size_t nvertices = line->numberOfVertices();
    kvs::ValueArray<kvs::Real32> coords( nvertices * 6 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        coords[ 6 * i + 0 ] = line->coords()[ 3 * i + 0 ];
        coords[ 6 * i + 1 ] = line->coords()[ 3 * i + 1 ];
        coords[ 6 * i + 2 ] = line->coords()[ 3 * i + 2 ];
        coords[ 6 * i + 3 ] = line->coords()[ 3 * i + 0 ];
        coords[ 6 * i + 4 ] = line->coords()[ 3 * i + 1 ];
        coords[ 6 * i + 5 ] = line->coords()[ 3 * i + 2 ];
    }
    return coords;
}

inline kvs::ValueArray<kvs::UInt8> QuadVertexColors( const kvs::LineObject* line )
{
    const size_t nvertices = line->numberOfVertices();
    kvs::ValueArray<kvs::UInt8> colors( nvertices * 6 );
    if ( line->colorType() == kvs::LineObject::VertexColor )
    {
        for ( size_t i = 0; i < nvertices; i++ )
        {
            colors[ 6 * i + 0 ] = line->colors()[ 3 * i + 0 ];
            colors[ 6 * i + 1 ] = line->colors()[ 3 * i + 1 ];
            colors[ 6 * i + 2 ] = line->colors()[ 3 * i + 2 ];
            colors[ 6 * i + 3 ] = line->colors()[ 3 * i + 0 ];
            colors[ 6 * i + 4 ] = line->colors()[ 3 * i + 1 ];
            colors[ 6 * i + 5 ] = line->colors()[ 3 * i + 2 ];
        }
    }
    else
    {
        const kvs::RGBColor color = line->color();
        for ( size_t i = 0; i < nvertices; i++ )
        {
            colors[ 6 * i + 0 ] = color.r();
            colors[ 6 * i + 1 ] = color.g();
            colors[ 6 * i + 2 ] = color.b();
            colors[ 6 * i + 3 ] = color.r();
            colors[ 6 * i + 4 ] = color.g();
            colors[ 6 * i + 5 ] = color.b();
        }
    }
    return colors;
}

inline kvs::ValueArray<kvs::Real32> QuadVertexNormals( const kvs::LineObject* line )
{
    const size_t nvertices = line->numberOfVertices();
    kvs::ValueArray<kvs::Real32> normals( nvertices * 6 );

    if ( line->coords().size() == line->normals().size() )
    {
        for ( size_t i = 0; i < nvertices; i++ )
        {
            normals[ 6 * i + 0 ] = line->normals()[ 3 * i + 0 ];
            normals[ 6 * i + 1 ] = line->normals()[ 3 * i + 1 ];
            normals[ 6 * i + 2 ] = line->normals()[ 3 * i + 2 ];
            normals[ 6 * i + 3 ] = line->normals()[ 3 * i + 0 ];
            normals[ 6 * i + 4 ] = line->normals()[ 3 * i + 1 ];
            normals[ 6 * i + 5 ] = line->normals()[ 3 * i + 2 ];
        }
    }
    else
    {
        switch ( line->lineType() )
        {
        case kvs::LineObject::Uniline:
        {
            const size_t nconnections = line->numberOfConnections();
            for ( size_t i = 0; i < nconnections - 1; i++ )
            {
                const size_t id0 = line->connections().at( i );
                const size_t id1 = line->connections().at( i + 1 );
                const kvs::Vec3 p0 = line->coord( id0 );
                const kvs::Vec3 p1 = line->coord( id1 );
                const kvs::Vec3 n = ( p1 - p0 ).normalized();
                normals[ 6 * id0 + 0 ] = n.x();
                normals[ 6 * id0 + 1 ] = n.y();
                normals[ 6 * id0 + 2 ] = n.z();
                normals[ 6 * id0 + 3 ] = n.x();
                normals[ 6 * id0 + 4 ] = n.y();
                normals[ 6 * id0 + 5 ] = n.z();
                if ( i == nconnections - 2 )
                {
                    normals[ 6 * id1 + 0 ] = n.x();
                    normals[ 6 * id1 + 1 ] = n.y();
                    normals[ 6 * id1 + 2 ] = n.z();
                    normals[ 6 * id1 + 3 ] = n.x();
                    normals[ 6 * id1 + 4 ] = n.y();
                    normals[ 6 * id1 + 5 ] = n.z();
                }
            }
            break;
        }
        case kvs::LineObject::Segment:
        {
            const size_t nconnections = line->numberOfConnections();
            for ( size_t i = 0; i < nconnections; i++ )
            {
                const size_t id0 = line->connections().at( 2 * i );
                const size_t id1 = line->connections().at( 2 * i + 1 );
                const kvs::Vec3 p0 = line->coord( id0 );
                const kvs::Vec3 p1 = line->coord( id1 );
                const kvs::Vec3 n = ( p1 - p0 ).normalized();
                normals[ 6 * id0 + 0 ] = n.x();
                normals[ 6 * id0 + 1 ] = n.y();
                normals[ 6 * id0 + 2 ] = n.z();
                normals[ 6 * id0 + 3 ] = n.x();
                normals[ 6 * id0 + 4 ] = n.y();
                normals[ 6 * id0 + 5 ] = n.z();
                normals[ 6 * id1 + 0 ] = n.x();
                normals[ 6 * id1 + 1 ] = n.y();
                normals[ 6 * id1 + 2 ] = n.z();
                normals[ 6 * id1 + 3 ] = n.x();
                normals[ 6 * id1 + 4 ] = n.y();
                normals[ 6 * id1 + 5 ] = n.z();
            }
            break;
        }
        case kvs::LineObject::Polyline:
        {
            const size_t nconnections = line->numberOfConnections();
            for ( size_t i = 0; i < nconnections; i++ )
            {
                const size_t id0 = line->connections().at( 2 * i );
                const size_t id1 = line->connections().at( 2 * i + 1 );
                for ( size_t j = id0; j < id1; j++ )
                {
                    const kvs::Vec3 p0 = line->coord( j );
                    const kvs::Vec3 p1 = line->coord( j + 1 );
                    const kvs::Vec3 n = ( p1 - p0 ).normalized();
                    normals[ 6 * j + 0 ] = n.x();
                    normals[ 6 * j + 1 ] = n.y();
                    normals[ 6 * j + 2 ] = n.z();
                    normals[ 6 * j + 3 ] = n.x();
                    normals[ 6 * j + 4 ] = n.y();
                    normals[ 6 * j + 5 ] = n.z();
                    if ( j == id1 - 1 )
                    {
                        normals[ 6 * ( j + 1 ) + 0 ] = n.x();
                        normals[ 6 * ( j + 1 ) + 1 ] = n.y();
                        normals[ 6 * ( j + 1 ) + 2 ] = n.z();
                        normals[ 6 * ( j + 1 ) + 3 ] = n.x();
                        normals[ 6 * ( j + 1 ) + 4 ] = n.y();
                        normals[ 6 * ( j + 1 ) + 5 ] = n.z();
                    }
                }
            }
            break;
        }
        case kvs::LineObject::Strip:
        {
            const size_t nvertices = line->numberOfVertices();
            for ( size_t i = 0; i < nvertices - 1; i++ )
            {
                const kvs::Vec3 p0 = line->coord( i );
                const kvs::Vec3 p1 = line->coord( i + 1 );
                const kvs::Vec3 n = ( p1 - p0 ).normalized();
                normals[ 6 * i + 0 ] = n.x();
                normals[ 6 * i + 1 ] = n.y();
                normals[ 6 * i + 2 ] = n.z();
                normals[ 6 * i + 3 ] = n.x();
                normals[ 6 * i + 4 ] = n.y();
                normals[ 6 * i + 5 ] = n.z();
                if ( i == nvertices - 2 )
                {
                    normals[ 6 * ( i + 1 ) + 0 ] = n.x();
                    normals[ 6 * ( i + 1 ) + 1 ] = n.y();
                    normals[ 6 * ( i + 1 ) + 2 ] = n.z();
                    normals[ 6 * ( i + 1 ) + 3 ] = n.x();
                    normals[ 6 * ( i + 1 ) + 4 ] = n.y();
                    normals[ 6 * ( i + 1 ) + 5 ] = n.z();
                }
            }
            break;
        }
        default: break;
        }
    }

    return normals;
}

inline kvs::ValueArray<kvs::Real32> QuadVertexTexCoords(
    const kvs::LineObject* line,
    const float halo_size,
    const float radius_size )
{
    const size_t nvertices = line->numberOfVertices();
    kvs::ValueArray<kvs::Real32> texcoords( nvertices * 4 * 2 );

    const float halo_factor = 1.0f + 2.0f * halo_size;
    const float rot = 0.0f;
    const float zdiff = 0.0f;
    for ( size_t i = 0; i < nvertices; i++ )
    {
        texcoords[ 8 * i + 0 ] = -radius_size * halo_factor;
        texcoords[ 8 * i + 1 ] =  radius_size;
        texcoords[ 8 * i + 2 ] =  rot;
        texcoords[ 8 * i + 3 ] =  zdiff;
        texcoords[ 8 * i + 4 ] =  radius_size * halo_factor;
        texcoords[ 8 * i + 5 ] =  radius_size;
        texcoords[ 8 * i + 6 ] =  rot;
        texcoords[ 8 * i + 7 ] =  zdiff;
    }
    return texcoords;
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

SSAOStochasticStylizedLineRenderer::SSAOStochasticStylizedLineRenderer():
    StochasticRendererBase( new Engine() )
{
}

void SSAOStochasticStylizedLineRenderer::setOpacity( const kvs::UInt8 opacity )
{
    static_cast<Engine&>( engine() ).setOpacity( opacity );
}

void SSAOStochasticStylizedLineRenderer::setRadiusSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setRadiusSize( size );
}

void SSAOStochasticStylizedLineRenderer::setHaloSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setHaloSize( size );
}

void SSAOStochasticStylizedLineRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

void SSAOStochasticStylizedLineRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

kvs::UInt8 SSAOStochasticStylizedLineRenderer::opacity() const
{
    return static_cast<const Engine&>( engine() ).opacity();
}

kvs::Real32 SSAOStochasticStylizedLineRenderer::radiusSize() const
{
    return static_cast<const Engine&>( engine() ).radiusSize();
}

kvs::Real32 SSAOStochasticStylizedLineRenderer::haloSize() const
{
    return static_cast<const Engine&>( engine() ).haloSize();
}

kvs::Real32 SSAOStochasticStylizedLineRenderer::samplingSphereRadius()
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

size_t SSAOStochasticStylizedLineRenderer::numberOfSamplingPoints()
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

SSAOStochasticStylizedLineRenderer::Engine::Engine():
    m_line_opacity( 255 ),
    m_radius_size( 0.05f ),
    m_halo_size( 0.0f ),
    m_sampling_sphere_radius( 0.5f ),
    m_nsamples( 256 )
{
}

void SSAOStochasticStylizedLineRenderer::Engine::release()
{
    m_shader_geom_pass.release();
    m_shader_occl_pass.release();
    m_vbo_manager.release();
}

void SSAOStochasticStylizedLineRenderer::Engine::create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::LineObject* line = kvs::LineObject::DownCast( object );
    attachObject( line );
    createRandomTexture();
    this->create_shader_program();
    this->create_buffer_object( line );
    this->create_shape_texture();
    this->create_diffuse_texture();
    this->create_framebuffer( camera->windowWidth(), camera->windowHeight() );
    this->create_sampling_points();
}

void SSAOStochasticStylizedLineRenderer::Engine::update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    this->update_framebuffer( camera->windowWidth(), camera->windowHeight() );
}

void SSAOStochasticStylizedLineRenderer::Engine::setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 P = kvs::OpenGL::ProjectionMatrix();
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
    m_shader_geom_pass.bind();
    m_shader_geom_pass.setUniform( "ModelViewMatrix", M );
    m_shader_geom_pass.setUniform( "ProjectionMatrix", P );
    m_shader_geom_pass.setUniform( "NormalMatrix", N );
    m_shader_geom_pass.setUniform( "shape_texture", 0 );
    m_shader_geom_pass.setUniform( "diffuse_texture", 1 );
    m_shader_geom_pass.setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
    m_shader_geom_pass.setUniform( "random_texture", 2 );
    m_shader_geom_pass.setUniform( "opacity", m_line_opacity / 255.0f );
    m_shader_geom_pass.unbind();
}

void SSAOStochasticStylizedLineRenderer::Engine::draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    this->render_occlusion_pass();
}

void SSAOStochasticStylizedLineRenderer::Engine::preDraw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::LineObject* line = kvs::LineObject::DownCast( object );
    this->render_geometry_pass( line );
}

void SSAOStochasticStylizedLineRenderer::Engine::create_shader_program()
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( "SSAO_stochastic_stylized_geom_pass.vert" );
        kvs::ShaderSource frag( "SSAO_stochastic_stylized_geom_pass.frag" );
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

void SSAOStochasticStylizedLineRenderer::Engine::create_buffer_object( const kvs::LineObject* line )
{
    if ( line->numberOfColors() != 1 && line->colorType() == kvs::LineObject::LineColor )
    {
        kvsMessageError("Not supported line color type.");
        return;
    }

    const kvs::ValueArray<kvs::UInt16> indices = ::RandomIndices( line, randomTextureSize() );
    const kvs::ValueArray<kvs::Real32> coords = ::QuadVertexCoords( line );
    const kvs::ValueArray<kvs::UInt8> colors = ::QuadVertexColors( line );
    const kvs::ValueArray<kvs::Real32> normals = ::QuadVertexNormals( line );
    const kvs::ValueArray<kvs::Real32> texcoords = ::QuadVertexTexCoords( line, m_halo_size, m_radius_size );
    m_vbo_manager.setVertexAttribArray( indices, m_shader_geom_pass.attributeLocation("random_index"), 2 );
    m_vbo_manager.setVertexArray( coords, 3 );
    m_vbo_manager.setColorArray( colors, 3 );
    m_vbo_manager.setNormalArray( normals );
    m_vbo_manager.setTexCoordArray( texcoords, 4 );
    m_vbo_manager.create();

    if ( line->lineType() == kvs::LineObject::Polyline )
    {
        const kvs::UInt32* pconnections = line->connections().data();
        m_first_array.allocate( line->numberOfConnections() );
        m_count_array.allocate( m_first_array.size() );
        for ( size_t i = 0; i < m_first_array.size(); ++i )
        {
            m_first_array[i] = 2 * ( pconnections[ 2 * i ] );
            m_count_array[i] = 2 * ( pconnections[ 2 * i + 1 ] - pconnections[ 2 * i ] + 1 );
        }
    }
}

void SSAOStochasticStylizedLineRenderer::Engine::create_shape_texture()
{
    const size_t resolution = 256;
    kvs::ValueArray<kvs::Real32> shape( resolution * resolution * 4 );
    for ( size_t j = 0, index = 0; j < resolution; j++ )
    {
        for ( size_t i = 0; i < resolution; i++, index++ )
        {
            const size_t index4 = index * 4;
            const kvs::Real32 x = ( i * 2.0f ) / kvs::Real32( resolution ) - 1.0f;

            // Cylinder shape.
            shape[ index4 + 0 ] = x * 0.5f + 0.5f;
            shape[ index4 + 1 ] = std::sqrt( 1.0f - x * x );
            shape[ index4 + 2 ] = std::sqrt( 1.0f - x * x );
        }
    }

    m_shape_texture.setWrapS( GL_REPEAT );
    m_shape_texture.setWrapT( GL_REPEAT );
    m_shape_texture.setMagFilter( GL_NEAREST );
    m_shape_texture.setMinFilter( GL_NEAREST );
    m_shape_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_shape_texture.create( resolution, resolution, shape.data() );
}

void SSAOStochasticStylizedLineRenderer::Engine::create_diffuse_texture()
{
/*
    const int resolution = 256;
    kvs::ValueArray<kvs::Real32> diffuse( resolution * resolution * 4 );
    for ( int j = 0, index = 0; j < resolution; j++ )
    {
        for ( int i = 0; i < resolution; i++, index++ )
        {
            const size_t index4 = index * 4;
//            const size_t index4 = ( j * resolution + i ) * 4;
//            const float x = kvs::Real32(i) / kvs::Real32( resolution );
            const float y = kvs::Real32(j) / kvs::Real32( resolution );

            if ( abs( ( resolution / 2 ) - i ) > ( j % ( resolution / 4 ) ) )
            {
                diffuse[ index4 + 0 ] = 1.0f;
                diffuse[ index4 + 1 ] = 1.0f;
                diffuse[ index4 + 2 ] = 1.0f;
            }
            else
            {
                diffuse[ index4 + 0 ] = 0.5f;
                diffuse[ index4 + 1 ] = 0.5f;
                diffuse[ index4 + 2 ] = 0.5f;
            }
            diffuse[ index4 + 3 ] = 1.0f;

//            diffuse[ index4 + 0 ] = (y * 4.0f) - (float)(int)(y*4.0f);
//            diffuse[ index4 + 1 ] = (y * 4.0f) - (float)(int)(y*4.0f);
//            diffuse[ index4 + 2 ] = (y * 4.0f) - (float)(int)(y*4.0f);
//            diffuse[ index4 + 3 ] = 1.0f;
        }
    }
    m_diffuse_texture.setWrapS( GL_REPEAT );
    m_diffuse_texture.setWrapT( GL_REPEAT );
    m_diffuse_texture.setMagFilter( GL_LINEAR );
    m_diffuse_texture.setMinFilter( GL_LINEAR );
    m_diffuse_texture.setPixelFormat( GL_RGBA32F, GL_RGBA, GL_FLOAT );
    m_diffuse_texture.create( resolution, resolution, diffuse.data() );
*/

    kvs::ValueArray<kvs::UInt8> diffuse( 3 );
    diffuse[0] = 255;
    diffuse[1] = 255;
    diffuse[2] = 255;

    m_diffuse_texture.setWrapS( GL_REPEAT );
    m_diffuse_texture.setWrapT( GL_REPEAT );
    m_diffuse_texture.setMagFilter( GL_LINEAR );
    m_diffuse_texture.setMinFilter( GL_LINEAR );
    m_diffuse_texture.setPixelFormat( GL_RGB, GL_RGB, GL_UNSIGNED_BYTE );
    m_diffuse_texture.create( 1, 1, diffuse.data() );
}

void SSAOStochasticStylizedLineRenderer::Engine::create_framebuffer( const size_t width, const size_t height )
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

void SSAOStochasticStylizedLineRenderer::Engine::create_sampling_points()
{
    const size_t nsamples = m_nsamples;
    const float radius = m_sampling_sphere_radius;
    const size_t dim = 3;
    const kvs::ValueArray<GLfloat> sampling_points = AmbientOcclusionRendering::SSAOPointSampling( radius, nsamples );
    m_shader_occl_pass.bind();
    m_shader_occl_pass.setUniform( "sampling_points", sampling_points, dim );
    m_shader_occl_pass.unbind();
}

void SSAOStochasticStylizedLineRenderer::Engine::update_framebuffer( const size_t width, const size_t height )
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

void SSAOStochasticStylizedLineRenderer::Engine::render_geometry_pass( const kvs::LineObject* line )
{
    kvs::FrameBufferObject::Binder bind0( m_framebuffer );

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
    kvs::Texture::Binder unit0( m_shape_texture, 0 );
    kvs::Texture::Binder unit1( m_diffuse_texture, 1 );
    kvs::Texture::Binder unit2( randomTexture(), 2 );
    {
        kvs::Texture::SetEnv( GL_TEXTURE_ENV_MODE, GL_REPLACE );
        kvs::OpenGL::WithEnabled d( GL_DEPTH_TEST );

        const size_t size = randomTextureSize();
        const int count = repetitionCount() * ::RandomNumber();
        const float offset_x = static_cast<float>( ( count ) % size );
        const float offset_y = static_cast<float>( ( count / size ) % size );
        const kvs::Vec2 random_offset( offset_x, offset_y );
        m_shader_geom_pass.setUniform( "random_offset", random_offset );

        // Draw lines.
        switch ( line->lineType() )
        {
        case kvs::LineObject::Polyline:
            m_vbo_manager.drawArrays( GL_QUAD_STRIP, m_first_array, m_count_array );
            break;
        case kvs::LineObject::Strip:
            m_vbo_manager.drawArrays( GL_QUAD_STRIP, 0, line->numberOfVertices() * 2 );
            break;
        default:
            break;
        }
    }
}

void SSAOStochasticStylizedLineRenderer::Engine::render_occlusion_pass()
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
