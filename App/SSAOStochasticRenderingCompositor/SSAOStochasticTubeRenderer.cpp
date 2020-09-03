#include "SSAOStochasticTubeRenderer.h"
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

inline kvs::ValueArray<kvs::Real32> QuadVertexValues( const kvs::LineObject* line )
{
    const size_t nvertices = line->numberOfVertices();
    kvs::ValueArray<kvs::Real32> values( nvertices * 2 );
    for ( size_t i = 0; i < nvertices; i++ )
    {
        values[ 2 * i + 0 ] = line->sizes()[i];
        values[ 2 * i + 1 ] = line->sizes()[i];
    }
    return values;
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

}

namespace AmbientOcclusionRendering
{

SSAOStochasticTubeRenderer::SSAOStochasticTubeRenderer():
    SSAOStochasticRendererBase( new Engine() )
{
}

void SSAOStochasticTubeRenderer::setTransferFunction( const kvs::TransferFunction& tfunc )
{
    static_cast<Engine&>( engine() ).setTransferFunction( tfunc );
}

void SSAOStochasticTubeRenderer::setRadiusSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setRadiusSize( size );
}

void SSAOStochasticTubeRenderer::setHaloSize( const kvs::Real32 size )
{
    static_cast<Engine&>( engine() ).setHaloSize( size );
}

const kvs::TransferFunction& SSAOStochasticTubeRenderer::transferFunction() const
{
    return static_cast<const Engine&>( engine() ).transferFunction();
}

kvs::Real32 SSAOStochasticTubeRenderer::radiusSize() const
{
    return static_cast<const Engine&>( engine() ).radiusSize();
}

kvs::Real32 SSAOStochasticTubeRenderer::haloSize() const
{
    return static_cast<const Engine&>( engine() ).haloSize();
}

SSAOStochasticTubeRenderer::Engine::Engine():
    m_radius_size( 0.05f ),
    m_halo_size( 0.0f ),
    m_tfunc_changed( true )
{
}

void SSAOStochasticTubeRenderer::Engine::release()
{
    m_shader_geom_pass.release();
    m_vbo_manager.release();
    m_tfunc_changed = true;
}

void SSAOStochasticTubeRenderer::Engine::create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::LineObject* line = kvs::LineObject::DownCast( object );
    attachObject( line );
    createRandomTexture();
    this->create_shader_program();
    this->create_buffer_object( line );
    this->create_shape_texture();
    this->create_diffuse_texture();
    this->create_transfer_function_texture();
}

void SSAOStochasticTubeRenderer::Engine::update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
}

void SSAOStochasticTubeRenderer::Engine::setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    if ( m_tfunc_changed )
    {
        m_tfunc_texture.release();
        this->create_transfer_function_texture();
    }

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
    m_shader_geom_pass.setUniform( "transfer_function_texture", 3 );
    m_shader_geom_pass.unbind();
}

void SSAOStochasticTubeRenderer::Engine::draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    const kvs::LineObject* line = kvs::LineObject::DownCast( object );
    
    kvs::VertexBufferObjectManager::Binder bind1( m_vbo_manager );
    kvs::ProgramObject::Binder bind2( m_shader_geom_pass );
    kvs::Texture::Binder unit0( m_shape_texture, 0 );
    kvs::Texture::Binder unit1( m_diffuse_texture, 1 );
    kvs::Texture::Binder unit2( randomTexture(), 2 );
    kvs::Texture::Binder unit3( m_tfunc_texture, 3 );
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

void SSAOStochasticTubeRenderer::Engine::create_shader_program()
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( "SSAO_SR_tube_geom_pass.vert" );
        kvs::ShaderSource frag( "SSAO_SR_tube_geom_pass.frag" );
        m_shader_geom_pass.build( vert, frag );

        kvs::Real32 min_value = 0.0f;
        kvs::Real32 max_value = 0.0f;
        if ( m_tfunc.hasRange() )
        {
            min_value = m_tfunc.minValue();
            max_value = m_tfunc.maxValue();
        }
        else
        {
            const kvs::LineObject* line = kvs::LineObject::DownCast( object() );
            const kvs::ValueArray<kvs::Real32>& values = line->sizes();
            min_value = values[0];
            max_value = values[1];
            for ( size_t i = 0; i < values.size(); i++ )
            {
                min_value = kvs::Math::Min( min_value, values[i] );
                max_value = kvs::Math::Max( max_value, values[i] );
            }
        }

        m_shader_geom_pass.bind();
        m_shader_geom_pass.setUniform( "min_value", min_value );
        m_shader_geom_pass.setUniform( "max_value", max_value );
        m_shader_geom_pass.unbind();
    }
}

void SSAOStochasticTubeRenderer::Engine::create_buffer_object( const kvs::LineObject* line )
{
    if ( line->numberOfColors() != 1 && line->colorType() == kvs::LineObject::LineColor )
    {
        kvsMessageError("Not supported line color type.");
        return;
    }

    const kvs::ValueArray<kvs::UInt16> indices = ::RandomIndices( line, randomTextureSize() );
    const kvs::ValueArray<kvs::Real32> values = ::QuadVertexValues( line );
    const kvs::ValueArray<kvs::Real32> coords = ::QuadVertexCoords( line );
    const kvs::ValueArray<kvs::UInt8> colors = ::QuadVertexColors( line );
    const kvs::ValueArray<kvs::Real32> normals = ::QuadVertexNormals( line );
    const kvs::ValueArray<kvs::Real32> texcoords = ::QuadVertexTexCoords( line, m_halo_size, m_radius_size );
    m_vbo_manager.setVertexAttribArray( indices, m_shader_geom_pass.attributeLocation("random_index"), 2 );
    m_vbo_manager.setVertexAttribArray( values, m_shader_geom_pass.attributeLocation("value"), 1 );
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

void SSAOStochasticTubeRenderer::Engine::create_shape_texture()
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

void SSAOStochasticTubeRenderer::Engine::create_diffuse_texture()
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

void SSAOStochasticTubeRenderer::Engine::create_transfer_function_texture()
{
    const size_t width = m_tfunc.resolution();
    const kvs::ValueArray<kvs::Real32> table = m_tfunc.table();
    m_tfunc_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_tfunc_texture.setMagFilter( GL_LINEAR );
    m_tfunc_texture.setMinFilter( GL_LINEAR );
    m_tfunc_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT  );
    m_tfunc_texture.create( width, table.data() );
    m_tfunc_changed = false;
}

} // end of namespace AmbientOcclusionRendering
