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
        GLenum type; ///< GL_UNSIGNED_BYTE, GL_FLOAT, etc
        GLsizei size; ///< data size [bytes]
        GLint dim; ///< data dimension
        GLsizei stride; ///< data stride
        const GLvoid* pointer; ///< pointer to the data
        GLsizei offset; ///< offset bytes
        VertexBuffer():
            type(0),
            size(0),
            dim(0),
            stride(0),
            pointer(0),
            offset(0) {}
    };

    struct IndexBuffer
    {
        GLenum type; ///< GL_UNSIGNED_BYTE, GL_FLOAT, etc
        GLsizei size; ///< data size [bytes]
        const GLvoid* pointer; ///< pointer to the data
        IndexBuffer():
            type(0),
            size(0),
            pointer(0) {}
    };

public:
    class Binder;

private:
    kvs::VertexBufferObject m_vbo;
    kvs::IndexBufferObject m_ibo;
    size_t m_vbo_size;
    size_t m_ibo_size;

    VertexBuffer m_vertex_array;
    VertexBuffer m_color_array;
    VertexBuffer m_normal_array;
    VertexBuffer m_tex_coord_array;
    IndexBuffer m_index_array;

public:
    VertexBufferObjectManager();

    const kvs::VertexBufferObject& vertexBufferObject() const { return m_vbo; }
    const kvs::IndexBufferObject& indexBufferObject() const { return m_ibo; }
    void setVertexArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride = 0 );
    void setColorArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride = 0 );
    void setNormalArray( const kvs::AnyValueArray& array, const size_t stride = 0 );
    void setTexCoordArray( const kvs::AnyValueArray& array, const size_t dim, const size_t stride = 0 );
    void setIndexArray( const kvs::AnyValueArray& array );

    void create();
    void bind() const;
    void unbind() const;
    void release();

    void drawArrays( GLenum mode, GLint first, GLsizei count );
    void drawArrays( GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount );
    void drawArrays( GLenum mode, const kvs::ValueArray<GLint>& first, const kvs::ValueArray<GLsizei>& count );
    void drawElements( GLenum mode, GLsizei count );
    void drawElements( GLenum mode, const GLsizei* count, GLsizei drawcount );
    void drawElements( GLenum mode, const kvs::ValueArray<GLsizei>& count );

private:
    void enable_client_state() const;
    void disable_client_state() const;
};

class VertexBufferObjectManager::Binder
{
    const kvs::VertexBufferObjectManager& m_vbo_manager;

public:
    Binder( const kvs::VertexBufferObjectManager& vbo_manager );
    ~Binder();

private:
    Binder( const Binder& );
    Binder& operator = ( const Binder& );
};

} // end of namespace kvs
