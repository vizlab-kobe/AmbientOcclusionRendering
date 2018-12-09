#pragma once
#include <kvs/VertexBufferObject>
#include <kvs/IndexBufferObject>
#include <kvs/AnyValueArray>
#include <kvs/OpenGL>
#include <vector>


namespace kvs
{

class VertexBufferObjectManager
{
private:
    struct VertexBuffer
    {
        GLenum array_type; ///< GL_VERTEX_ARRAY, GL_COLOR_ARRAY, etc
        GLenum data_type; ///< GL_UNSIGNED_BYTE, GL_FLOAT, etc
        GLsizei data_size; ///< data size [bytes]
        GLint dim; ///< data dimension
        GLsizei stride; ///< data stride
        const GLvoid* pointer; ///< pointer to the data
        GLsizei offset; ///< offset bytes
    };

    struct IndexBuffer
    {
        GLenum data_type; ///< GL_UNSIGNED_BYTE, GL_FLOAT, etc
        GLsizei data_size; ///< data size [bytes]
        const GLvoid* pointer; ///< pointer to the data
        GLsizei offset; ///< offset bytes
    };

    typedef std::vector<VertexBuffer> VertexBuffers;
    typedef std::vector<IndexBuffer> IndexBuffers;

private:
    kvs::VertexBufferObject m_vbo;
    kvs::IndexBufferObject m_ibo;
    size_t m_vbo_size;
    size_t m_ibo_size;
    VertexBuffers m_vertex_buffers;
    IndexBuffers m_index_buffers;

public:
    VertexBufferObjectManager();

    void setVertexArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride = 0 );
    void setColorArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride = 0 );
    void setNormalArray( const kvs::AnyValueArray& array, const size_t stride = 0 );
    void setTexCoordArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride = 0 );
    void setIndexArray( const kvs::AnyValueArray& array );

    void create();
    void bind();
    void unbind();

private:
    void enable_client_state();
    void disable_client_state();
};

} // end of namespace kvs
