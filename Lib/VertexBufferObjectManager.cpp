#include "VertexBufferObjectManager.h"

namespace
{

inline GLenum GLType( const kvs::AnyValueArray& array )
{
    switch ( array.typeID() )
    {
    case kvs::Type::TypeInt8: return GL_BYTE;
    case kvs::Type::TypeInt16: return GL_SHORT;
    case kvs::Type::TypeInt32: return GL_INT;
    case kvs::Type::TypeUInt8: return GL_UNSIGNED_BYTE;
    case kvs::Type::TypeUInt16: return GL_UNSIGNED_SHORT;
    case kvs::Type::TypeUInt32: return GL_UNSIGNED_INT;
    case kvs::Type::TypeReal32: return GL_FLOAT;
    case kvs::Type::TypeReal64: return GL_DOUBLE;
    default: break;
    }
    return kvs::Type::UnknownType;
}

}

namespace kvs
{

VertexBufferObjectManager::VertexBufferObjectManager():
    m_vbo_size( 0 ),
    m_ibo_size( 0 )
{
}

void VertexBufferObjectManager::setVertexArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride )
{
    m_vertex_array.type = ::GLType( array );
    m_vertex_array.size = array.byteSize();
    m_vertex_array.dim = dim;
    m_vertex_array.stride = stride;
    m_vertex_array.pointer = array.data();
}

void VertexBufferObjectManager::setColorArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride )
{
    m_color_array.type = ::GLType( array );
    m_color_array.size = array.byteSize();
    m_color_array.dim = dim;
    m_color_array.stride = stride;
    m_color_array.pointer = array.data();
}

void VertexBufferObjectManager::setNormalArray( const kvs::AnyValueArray& array, const size_t stride )
{
    m_normal_array.type = ::GLType( array );
    m_normal_array.size = array.byteSize();
    m_normal_array.dim = 3;
    m_normal_array.stride = stride;
    m_normal_array.pointer = array.data();
}

void VertexBufferObjectManager::setTexCoordArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride )
{
    m_tex_coord_array.type = ::GLType( array );
    m_tex_coord_array.size = array.byteSize();
    m_tex_coord_array.dim = dim;
    m_tex_coord_array.stride = stride;
    m_tex_coord_array.pointer = array.data();
}

void VertexBufferObjectManager::setIndexArray( const kvs::AnyValueArray& array )
{
    m_index_array.type = ::GLType( array );
    m_index_array.size = array.byteSize();
    m_index_array.pointer = array.data();
}

void VertexBufferObjectManager::create()
{
    const size_t vbo_size = m_vertex_array.size + m_color_array.size + m_normal_array.size + m_tex_coord_array.size;
    if ( vbo_size > 0 )
    {
        m_vbo.create( vbo_size );
        {
            size_t offset = 0;
            m_vbo.bind();
            if ( m_vertex_array.size > 0 )
            {
                m_vbo.load( m_vertex_array.size, m_vertex_array.pointer, offset );
                m_vertex_array.offset = offset;
                offset += m_vertex_array.size;
            }

            if ( m_color_array.size > 0 )
            {
                m_vbo.load( m_color_array.size, m_color_array.pointer, offset );
                m_color_array.offset = offset;
                offset += m_color_array.size;
            }

            if ( m_normal_array.size > 0 )
            {
                m_vbo.load( m_normal_array.size, m_normal_array.pointer, offset );
                m_normal_array.offset = offset;
                offset += m_normal_array.size;
            }

            if ( m_tex_coord_array.size > 0 )
            {
                m_vbo.load( m_tex_coord_array.size, m_tex_coord_array.pointer, offset );
                m_tex_coord_array.offset = offset;
                offset += m_tex_coord_array.size;
            }
            m_vbo.unbind();
        }

        const size_t ibo_size = m_index_array.size;
        if ( ibo_size > 0 )
        {
            m_ibo.create( ibo_size );
            m_ibo.bind();
            m_ibo.load( m_index_array.size, m_index_array.pointer, 0 );
            m_ibo.unbind();
        }
    }
}

void VertexBufferObjectManager::bind() const
{
    m_vbo.bind();
    this->enable_client_state();
}

void VertexBufferObjectManager::unbind() const
{
    this->disable_client_state();
    m_vbo.unbind();
}

void VertexBufferObjectManager::release()
{
    m_vbo.release();
    m_ibo.release();
}

void VertexBufferObjectManager::drawArrays( GLenum mode, GLint first, GLsizei count )
{
    kvs::OpenGL::DrawArrays( mode, first, count );
}

void VertexBufferObjectManager::drawArrays( GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount )
{
    kvs::OpenGL::MultiDrawArrays( mode, first, count, drawcount );
}

void VertexBufferObjectManager::drawArrays( GLenum mode, const kvs::ValueArray<GLint>& first, const kvs::ValueArray<GLsizei>& count )
{
    kvs::OpenGL::MultiDrawArrays( mode, first, count );
}

void VertexBufferObjectManager::drawElements( GLenum mode, GLsizei count )
{
    kvs::IndexBufferObject::Binder bind( m_ibo );
    kvs::OpenGL::DrawElements( mode, count, m_index_array.type, 0 );
}

void VertexBufferObjectManager::drawElements( GLenum mode, const GLsizei* count, GLsizei drawcount )
{
    kvs::IndexBufferObject::Binder bind( m_ibo );
    kvs::OpenGL::MultiDrawElements( mode, count, m_index_array.type, 0, drawcount );
}

void VertexBufferObjectManager::drawElements( GLenum mode, const kvs::ValueArray<GLsizei>& count )
{
    kvs::IndexBufferObject::Binder bind( m_ibo );
    kvs::OpenGL::MultiDrawElements( mode, count, m_index_array.type, 0 );
}

void VertexBufferObjectManager::enable_client_state() const
{
    if ( m_vertex_array.size > 0 )
    {
        const VertexBuffer& array = m_vertex_array;
        kvs::OpenGL::EnableClientState( GL_VERTEX_ARRAY );
        kvs::OpenGL::VertexPointer( array.dim, array.type, array.stride, (GLbyte*)NULL + array.offset );
    }

    if ( m_color_array.size > 0 )
    {
        const VertexBuffer& array = m_color_array;
        kvs::OpenGL::EnableClientState( GL_COLOR_ARRAY );
        kvs::OpenGL::ColorPointer( array.dim, array.type, array.stride, (GLbyte*)NULL + array.offset );
    }

    if ( m_normal_array.size > 0 )
    {
        const VertexBuffer& array = m_normal_array;
        kvs::OpenGL::EnableClientState( GL_NORMAL_ARRAY );
        kvs::OpenGL::NormalPointer( array.type, array.stride, (GLbyte*)NULL + array.offset );
    }

    if ( m_tex_coord_array.size > 0 )
    {
        const VertexBuffer& array = m_tex_coord_array;
        kvs::OpenGL::EnableClientState( GL_TEXTURE_COORD_ARRAY );
        kvs::OpenGL::TexCoordPointer( array.dim, array.type, array.stride, (GLbyte*)NULL + array.offset );
    }
}

void VertexBufferObjectManager::disable_client_state() const
{
    if ( m_vertex_array.size > 0 )
    {
        kvs::OpenGL::DisableClientState( GL_VERTEX_ARRAY );
    }

    if ( m_color_array.size > 0 )
    {
        kvs::OpenGL::DisableClientState( GL_COLOR_ARRAY );
    }

    if ( m_normal_array.size > 0 )
    {
        kvs::OpenGL::DisableClientState( GL_NORMAL_ARRAY );
    }

    if ( m_tex_coord_array.size > 0 )
    {
        kvs::OpenGL::DisableClientState( GL_TEXTURE_COORD_ARRAY );
    }
}

VertexBufferObjectManager::Binder::Binder( const kvs::VertexBufferObjectManager& vbo_manager ):
    m_vbo_manager( vbo_manager )
{
    m_vbo_manager.bind();
}

VertexBufferObjectManager::Binder::~Binder()
{
    m_vbo_manager.unbind();
}

} // end of namespace kvs
