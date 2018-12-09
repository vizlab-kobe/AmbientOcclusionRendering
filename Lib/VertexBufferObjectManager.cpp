#include "VertexBufferObjectManager.h"

namespace
{

GLenum GLType( const kvs::AnyValueArray& array )
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
    VertexBuffer buffer;
    buffer.array_type = GL_VERTEX_ARRAY;
    buffer.data_type = ::GLType( array );
    buffer.data_size = array.byteSize();
    buffer.dim = dim;
    buffer.stride = stride;
    buffer.pointer = array.data();
    buffer.offset = m_vbo_size;

    m_vbo_size += array.byteSize();
    m_vertex_buffers.push_back( buffer );
}

void VertexBufferObjectManager::setColorArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride )
{
    VertexBuffer buffer;
    buffer.array_type = GL_COLOR_ARRAY;
    buffer.data_type = ::GLType( array );
    buffer.data_size = array.byteSize();
    buffer.dim = dim;
    buffer.stride = stride;
    buffer.pointer = array.data();
    buffer.offset = m_vbo_size;

    m_vbo_size += array.byteSize();
    m_vertex_buffers.push_back( buffer );
}

void VertexBufferObjectManager::setNormalArray( const kvs::AnyValueArray& array, const size_t stride )
{
    VertexBuffer buffer;
    buffer.array_type = GL_NORMAL_ARRAY;
    buffer.data_type = ::GLType( array );
    buffer.data_size = array.byteSize();
    buffer.dim = 3;
    buffer.stride = stride;
    buffer.pointer = array.data();
    buffer.offset = m_vbo_size;

    m_vbo_size += array.byteSize();
    m_vertex_buffers.push_back( buffer );
}

void VertexBufferObjectManager::setTexCoordArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride )
{
    VertexBuffer buffer;
    buffer.array_type = GL_TEXTURE_COORD_ARRAY;
    buffer.data_type = ::GLType( array );
    buffer.data_size = array.byteSize();
    buffer.dim = dim;
    buffer.stride = stride;
    buffer.pointer = array.data();
    buffer.offset = m_vbo_size;

    m_vbo_size += array.byteSize();
    m_vertex_buffers.push_back( buffer );
}

void VertexBufferObjectManager::setIndexArray( const kvs::AnyValueArray& array )
{
    IndexBuffer buffer;
    buffer.data_type = ::GLType( array );
    buffer.data_size = array.byteSize();
    buffer.pointer = array.data();
    buffer.offset = m_ibo_size;

    m_ibo_size += array.byteSize();
    m_index_buffers.push_back( buffer );
}

void VertexBufferObjectManager::create()
{
    if ( m_vbo_size > 0 )
    {
        m_vbo.create( m_vbo_size );
        for ( size_t i = 0; i < m_vertex_buffers.size(); i++ )
        {
            const VertexBuffer& buffer = m_vertex_buffers[i];
            m_vbo.load( buffer.data_size, buffer.pointer, buffer.offset );
        }
    }

    if ( m_ibo_size > 0 )
    {
        m_ibo.create( m_ibo_size );
        for ( size_t i = 0; i < m_index_buffers.size(); i++ )
        {
            const IndexBuffer& buffer = m_index_buffers[i];
            m_ibo.load( buffer.data_size, buffer.pointer, buffer.offset );
        }
    }
}

void VertexBufferObjectManager::bind()
{
    if ( !m_vbo.isBound() )
    {
        m_vbo.bind();
        if ( !m_ibo.isBound() )
        {
            m_ibo.bind();
        }

        this->enable_client_state();
    }
}

void VertexBufferObjectManager::unbind()
{
    if ( m_ibo.isBound() )
    {
        this->disable_client_state();

        m_ibo.unbind();
        if ( m_vbo.isBound() )
        {
            m_vbo.unbind();
        }
    }
}

void VertexBufferObjectManager::enable_client_state()
{
    for ( size_t i = 0; i < m_vertex_buffers.size(); i++ )
    {
        const VertexBuffer& buffer = m_vertex_buffers[i];
        switch ( buffer.array_type )
        {
        case GL_VERTEX_ARRAY:
        {
            kvs::OpenGL::EnableClientState( GL_VERTEX_ARRAY );
            kvs::OpenGL::VertexPointer( buffer.dim, buffer.data_type, buffer.stride, (GLbyte*)NULL + buffer.offset );
            break;
        }
        case GL_COLOR_ARRAY:
        {
            kvs::OpenGL::EnableClientState( GL_COLOR_ARRAY );
            kvs::OpenGL::ColorPointer( buffer.dim, buffer.data_type, buffer.stride, (GLbyte*)NULL + buffer.offset );
            break;
        }
        case GL_NORMAL_ARRAY:
        {
            kvs::OpenGL::EnableClientState( GL_NORMAL_ARRAY );
            kvs::OpenGL::NormalPointer( buffer.data_type, buffer.stride, (GLbyte*)NULL + buffer.offset );
            break;
        }
        case GL_TEXTURE_COORD_ARRAY:
        {
            kvs::OpenGL::EnableClientState( GL_TEXTURE_COORD_ARRAY );
            kvs::OpenGL::TexCoordPointer( buffer.dim, buffer.data_type, buffer.stride, (GLbyte*)NULL + buffer.offset );
            break;
        }
        default: break;
        }
    }
}

void VertexBufferObjectManager::disable_client_state()
{
    for ( size_t i = 0; i < m_vertex_buffers.size(); i++ )
    {
        const VertexBuffer& buffer = m_vertex_buffers[i];
        switch ( buffer.array_type )
        {
        case GL_VERTEX_ARRAY:
        {
            kvs::OpenGL::DisableClientState( GL_VERTEX_ARRAY );
            break;
        }
        case GL_COLOR_ARRAY:
        {
            kvs::OpenGL::DisableClientState( GL_COLOR_ARRAY );
            break;
        }
        case GL_NORMAL_ARRAY:
        {
            kvs::OpenGL::DisableClientState( GL_NORMAL_ARRAY );
            break;
        }
        case GL_TEXTURE_COORD_ARRAY:
        {
            kvs::OpenGL::DisableClientState( GL_TEXTURE_COORD_ARRAY );
            break;
        }
        default: break;
        }
    }
}

} // end of namespace kvs
