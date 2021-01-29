/*****************************************************************************/
/**
 *  @file   StochasticUniformGridRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include "SSAOStochasticUniformGridRenderer.h"
#include <cmath>
#include <cfloat>
#include <kvs/OpenGL>
#include <kvs/StructuredVolumeObject>
#include <kvs/TransferFunction>
#include <kvs/ColorMap>
#include <kvs/OpacityMap>
#include <kvs/ValueArray>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Coordinate>
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
 *  @brief  Normalizes value array.
 *  @param  volume [in] pointer to the volume object
 *  @param  min_value [in] minimum value of the volume data
 *  @param  max_value [in] maximum value of the volume data
 *  @return normalized value array
 */
/*===========================================================================*/
template <typename T>
kvs::AnyValueArray NormalizeValues(
    const kvs::StructuredVolumeObject* volume,
    const kvs::Real32 min_value,
    const kvs::Real32 max_value )
{
    const kvs::Real32 scale = 1.0f / ( max_value - min_value );
    const size_t nnodes = volume->numberOfNodes();
    const T* src = static_cast<const T*>( volume->values().data() );

    kvs::ValueArray<kvs::Real32> data( nnodes );
    kvs::Real32* dst = data.data();
    for ( size_t i = 0; i < nnodes; i++ )
    {
        *(dst++) = static_cast<kvs::Real32>(( *(src++) - min_value ) * scale);
    }

    return kvs::AnyValueArray( data );
}

/*===========================================================================*/
/**
 *  @brief  Returns unsigned value array converted from signed value array.
 *  @param  volume [in] pointer to the volume object
 *  @return unsigned value array
 */
/*===========================================================================*/
template <typename DstType, typename SrcType>
kvs::AnyValueArray SignedToUnsigned( const kvs::StructuredVolumeObject* volume )
{
    const SrcType min = kvs::Value<SrcType>::Min();
    const size_t nvalues = volume->values().size();
    const SrcType* src = static_cast<const SrcType*>( volume->values().data() );

    kvs::ValueArray<DstType> data( nvalues );
    DstType* dst = data.data();
    for ( size_t i = 0; i < nvalues; i++ )
    {
        *(dst++) = static_cast<DstType>( *(src++) - min );
    }

    return kvs::AnyValueArray( data );
}

} // end of namespace


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new StochasticUniformGridRenderer class.
 */
/*===========================================================================*/
SSAOStochasticUniformGridRenderer::SSAOStochasticUniformGridRenderer():
    StochasticRendererBase( new Engine() )
{
}

void SSAOStochasticUniformGridRenderer::setEdgeFactor( const float factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( factor );
}

/*===========================================================================*/
/**
 *  @brief  Sets a sampling step.
 *  @param  transfer_function [in] transfer function
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::setSamplingStep( const float step )
{
    static_cast<Engine&>( engine() ).setSamplingStep( step );
}

/*===========================================================================*/
/**
 *  @brief  Sets a transfer function.
 *  @param  transfer_function [in] transfer function
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::setTransferFunction(
    const kvs::TransferFunction& transfer_function )
{
    static_cast<Engine&>( engine() ).setTransferFunction( transfer_function );
}

const kvs::TransferFunction& SSAOStochasticUniformGridRenderer::transferFunction() const
{
    return static_cast<const Engine&>( engine() ).transferFunction();
}

float SSAOStochasticUniformGridRenderer::samplingStep() const
{
    return static_cast<const Engine&>( engine() ).samplingStep();
}

void SSAOStochasticUniformGridRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

void SSAOStochasticUniformGridRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

kvs::Real32 SSAOStochasticUniformGridRenderer::samplingSphereRadius() const
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

size_t SSAOStochasticUniformGridRenderer::numberOfSamplingPoints() const
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticUniformGridRenderer::Engine::Engine():
    m_edge_factor( 0.0f ),
    m_random_index( 0 ),
    m_step( 0.5f ),
    m_transfer_function_changed( true )
{
    m_ao_buffer.setGeometryPassShaderFiles(
        "SSAO_SR_uniform_grid_geom_pass.vert",
        "SSAO_SR_uniform_grid_geom_pass.frag" );
    m_ao_buffer.setOcclusionPassShaderFiles(
        "SSAO_SR_uniform_grid_occl_pass.vert",
        "SSAO_SR_uniform_grid_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::release()
{
    // Release transfer function resources
    m_transfer_function_texture.release();
    m_transfer_function_changed = true;

    // Release buffer object resources
    m_volume_texture.release();
    m_entry_texture.release();
    m_exit_texture.release();
    m_entry_exit_framebuffer.release();
    m_bounding_cube_buffer.release();
    m_bounding_cube_shader.release();

    // Release AO buffer resources
    m_ao_buffer.release();
}

/*===========================================================================*/
/**
 *  @brief  Create.
 *  @param  object [in] pointer to the volume object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* volume = kvs::StructuredVolumeObject::DownCast( object );

    BaseClass::attachObject( object );
    BaseClass::createRandomTexture();

    // Create shader program
    this->create_shader_program( volume );

    // Create framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    this->create_framebuffer( framebuffer_width, framebuffer_height );

    // Create buffer object
    this->create_volume_texture( volume );
    this->create_bounding_cube_buffer( volume );

    // Create transfer function textures
    this->create_transfer_function_texture();
}

/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    auto* volume = kvs::StructuredVolumeObject::DownCast( object );

    // Update shader program
    this->update_shader_program( volume );

    // Update framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    this->update_framebuffer( framebuffer_width, framebuffer_height );

    // Update buffer object
    this->update_volume_texture( volume );
    this->update_bounding_cube_buffer( volume );
}

/*===========================================================================*/
/**
 *  @brief  Set up.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    m_random_index = m_ao_buffer.geometryPassShader().attributeLocation("random_index");

    if ( m_transfer_function_changed )
    {
        m_transfer_function_texture.release();
        this->create_transfer_function_texture();
    }

    // Setup shader program
    m_ao_buffer.setupShaderProgram( this->shader() );

    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 PM = kvs::OpenGL::ProjectionMatrix() * M;
    const kvs::Mat4 PM_inverse = PM.inverted();
    {
        const float f = camera->back();
        const float n = camera->front();
        const float to_zw1 = ( f * n ) / ( f - n );
        const float to_zw2 = 0.5f * ( ( f + n ) / ( f - n ) ) + 0.5f;
        const float to_ze1 = 0.5f + 0.5f * ( ( f + n ) / ( f - n ) );
        const float to_ze2 = ( f - n ) / ( f * n );

        auto& shader = m_ao_buffer.geometryPassShader();
        kvs::ProgramObject::Binder bind( shader );
        shader.setUniform( "ModelViewProjectionMatrix", PM );
        shader.setUniform( "ModelViewProjectionMatrixInverse", PM_inverse );
        shader.setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
        shader.setUniform( "volume_data", 0 );
        shader.setUniform( "exit_points", 1 );
        shader.setUniform( "entry_points", 2 );
        shader.setUniform( "transfer_function_data", 3 );
        shader.setUniform( "random_texture", 4 );
        shader.setUniform( "to_zw1", to_zw1 );
        shader.setUniform( "to_zw2", to_zw2 );
        shader.setUniform( "to_ze1", to_ze1 );
        shader.setUniform( "to_ze2", to_ze2 );
        shader.setUniform( "edge_factor", m_edge_factor );
    }

    const kvs::Vec3 L = kvs::WorldCoordinate( light->position() ).toObjectCoordinate( object ).position();
    const kvs::Vec3 C = kvs::WorldCoordinate( camera->position() ).toObjectCoordinate( object ).position();
    {
        auto& shader = m_ao_buffer.occlusionPassShader();
        kvs::ProgramObject::Binder bind( shader );
        shader.setUniform( "light_position", L );
        shader.setUniform( "camera_position", C );
        shader.setUniform( "ModelViewMatrix", M );
    }

    kvs::ProgramObject::Binder unit0( m_bounding_cube_shader );
    kvs::FrameBufferObject::Binder unit1( m_entry_exit_framebuffer );
    m_bounding_cube_shader.setUniform( "ModelViewProjectionMatrix", PM );

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_CULL_FACE );
    kvs::OpenGL::Disable( GL_LIGHTING );

    // Draw the back face of the bounding cube for the entry points.
    kvs::OpenGL::SetDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    kvs::OpenGL::SetCullFace( GL_FRONT );
    this->draw_bounding_cube_buffer();

    // Draw the front face of the bounding cube for the entry points.
    kvs::OpenGL::SetDrawBuffer( GL_COLOR_ATTACHMENT1_EXT );
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    kvs::OpenGL::SetCullFace( GL_BACK );
    this->draw_bounding_cube_buffer();

    kvs::OpenGL::Disable( GL_CULL_FACE );
}

/*===========================================================================*/
/**
 *  @brief  Draw an ensemble.
 *  @param  object [in] pointer to the line object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    m_ao_buffer.bind();
    this->draw_buffer_object( kvs::StructuredVolumeObject::DownCast( object ) );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();
}

/*===========================================================================*/
/**
 *  @brief  Creates shader program.
 *  @param  volume [in] pointer to the structured volume object
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::create_shader_program(
    const kvs::StructuredVolumeObject* volume )
{
    m_ao_buffer.createShaderProgram( this->shader(), this->isEnabledShading() );

    // Build bounding cube shader.
    {
        kvs::ShaderSource vert( "RC_bounding_cube.vert" );
        kvs::ShaderSource frag( "RC_bounding_cube.frag" );
        m_bounding_cube_shader.build( vert, frag );
    }

    // Set uniform variables.
    const kvs::Vec3ui r = volume->resolution();
    const kvs::Real32 max_ngrids = static_cast<kvs::Real32>( kvs::Math::Max( r.x(), r.y(), r.z() ) );
    const kvs::Vec3 resolution( static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.z()) );
    const kvs::Vec3 ratio( r.x() / max_ngrids, r.y() / max_ngrids, r.z() / max_ngrids );
    const kvs::Vec3 reciprocal( 1.0f / r.x(), 1.0f / r.y(), 1.0f / r.z() );
    kvs::Real32 min_range = 0.0f;
    kvs::Real32 max_range = 0.0f;
    kvs::Real32 min_value = m_transfer_function.colorMap().minValue();
    kvs::Real32 max_value = m_transfer_function.colorMap().maxValue();
    const std::type_info& type = volume->values().typeInfo()->type();
    if ( type == typeid( kvs::UInt8 ) )
    {
        min_range = 0.0f;
        max_range = 255.0f;
        if ( !m_transfer_function.hasRange() )
        {
            min_value = 0.0f;
            max_value = 255.0f;
        }
    }
    else if ( type == typeid( kvs::Int8 ) )
    {
        min_range = static_cast<kvs::Real32>( kvs::Value<kvs::UInt8>::Min() );
        max_range = static_cast<kvs::Real32>( kvs::Value<kvs::UInt8>::Max() );
        if ( !m_transfer_function.hasRange() )
        {
            min_value = -128.0f;
            max_value = 127.0f;
        }
    }
    else if ( type == typeid( kvs::UInt16 ) )
    {
        min_range = static_cast<kvs::Real32>( kvs::Value<kvs::UInt16>::Min() );
        max_range = static_cast<kvs::Real32>( kvs::Value<kvs::UInt16>::Max() );
        if ( !m_transfer_function.hasRange() )
        {
            min_value = static_cast<kvs::Real32>( volume->minValue() );
            max_value = static_cast<kvs::Real32>( volume->maxValue() );
        }
    }
    else if ( type == typeid( kvs::Int16 ) )
    {
        min_range = static_cast<kvs::Real32>( kvs::Value<kvs::Int16>::Min() );
        max_range = static_cast<kvs::Real32>( kvs::Value<kvs::Int16>::Max() );
        if ( !m_transfer_function.hasRange() )
        {
            min_value = static_cast<kvs::Real32>( volume->minValue() );
            max_value = static_cast<kvs::Real32>( volume->maxValue() );
        }
    }
    else if ( type == typeid( kvs::UInt32 ) || type == typeid( kvs::Int32  ) || type == typeid( kvs::Real32 ) )
    {
        min_range = 0.0f;
        max_range = 1.0f;
        min_value = 0.0f;
        max_value = 1.0f;
    }
    else
    {
        kvsMessageError( "Not supported data type '%s'.", volume->values().typeInfo()->typeName() );
    }

    auto& shader = m_ao_buffer.geometryPassShader();
    shader.bind();
    shader.setUniform( "volume.resolution", resolution );
    shader.setUniform( "volume.resolution_ratio", ratio );
    shader.setUniform( "volume.resolution_reciprocal", reciprocal );
    shader.setUniform( "volume.min_range", min_range );
    shader.setUniform( "volume.max_range", max_range );
    shader.setUniform( "transfer_function.min_value", min_value );
    shader.setUniform( "transfer_function.max_value", max_value );
    shader.setUniform( "sampling_step", m_step );
    shader.unbind();
}

void SSAOStochasticUniformGridRenderer::Engine::update_shader_program(
    const kvs::StructuredVolumeObject* volume )
{
    m_ao_buffer.updateShaderProgram( this->shader(), this->isEnabledShading() );

    m_bounding_cube_shader.release();
    this->create_shader_program( volume );
}

/*===========================================================================*/
/**
 *  @brief  Create volume texture object.
 *  @param  volume [in] pointer to the structured volume object
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::create_volume_texture(
    const kvs::StructuredVolumeObject* volume )
{
    if ( volume->gridType() != kvs::StructuredVolumeObject::Uniform )
    {
        kvsMessageError("Not supported grid type.");
        return;
    }

    if ( volume->veclen() != 1 )
    {
        kvsMessageError("Not scalar volume.");
        return;
    }

    GLenum data_format = 0;
    GLenum data_type = 0;
    kvs::AnyValueArray data_value;
    switch ( volume->values().typeID() )
    {
    case kvs::Type::TypeUInt8:
    {
        data_format = GL_ALPHA8;
        data_type = GL_UNSIGNED_BYTE;
        data_value = volume->values();
        break;
    }
    case kvs::Type::TypeUInt16:
    {
        data_format = GL_ALPHA16;
        data_type = GL_UNSIGNED_SHORT;
        data_value = volume->values();
        break;
    }
    case kvs::Type::TypeInt8:
    {
        data_format = GL_ALPHA8;
        data_type = GL_UNSIGNED_BYTE;
        data_value = ::SignedToUnsigned<kvs::UInt8,kvs::Int8>( volume );
        break;
    }
    case kvs::Type::TypeInt16:
    {
        data_format = GL_ALPHA16;
        data_type = GL_UNSIGNED_SHORT;
        data_value = ::SignedToUnsigned<kvs::UInt16,kvs::Int16>( volume );
        break;
    }
    case kvs::Type::TypeUInt32:
    {
        data_format = GL_ALPHA;
        data_type = GL_FLOAT;
        kvs::Real32 min_value = static_cast<kvs::Real32>( volume->minValue() );
        kvs::Real32 max_value = static_cast<kvs::Real32>( volume->maxValue() );
        if ( m_transfer_function.hasRange() )
        {
            min_value = m_transfer_function.colorMap().minValue();
            max_value = m_transfer_function.colorMap().maxValue();
        }
        data_value = ::NormalizeValues<kvs::UInt32>( volume, min_value, max_value );
        break;
    }
    case kvs::Type::TypeInt32:
    {
        data_format = GL_ALPHA;
        data_type = GL_FLOAT;
        kvs::Real32 min_value = static_cast<kvs::Real32>( volume->minValue() );
        kvs::Real32 max_value = static_cast<kvs::Real32>( volume->maxValue() );
        if ( m_transfer_function.hasRange() )
        {
            min_value = m_transfer_function.colorMap().minValue();
            max_value = m_transfer_function.colorMap().maxValue();
        }
        data_value = ::NormalizeValues<kvs::Int32>( volume, min_value, max_value );
        break;
    }
    case kvs::Type::TypeReal32:
    {
        data_format = GL_ALPHA;
        data_type = GL_FLOAT;
        kvs::Real32 min_value = static_cast<kvs::Real32>( volume->minValue() );
        kvs::Real32 max_value = static_cast<kvs::Real32>( volume->maxValue() );
        if ( m_transfer_function.hasRange() )
        {
            min_value = m_transfer_function.colorMap().minValue();
            max_value = m_transfer_function.colorMap().maxValue();
        }
        data_value = ::NormalizeValues<kvs::Real32>( volume, min_value, max_value );
        break;
    }
    case kvs::Type::TypeReal64:
    {
        data_format = GL_ALPHA;
        data_type = GL_FLOAT;
        kvs::Real32 min_value = static_cast<kvs::Real32>( volume->minValue() );
        kvs::Real32 max_value = static_cast<kvs::Real32>( volume->maxValue() );
        if ( m_transfer_function.hasRange() )
        {
            min_value = m_transfer_function.colorMap().minValue();
            max_value = m_transfer_function.colorMap().maxValue();
        }
        data_value = ::NormalizeValues<kvs::Real64>( volume, min_value, max_value );
        break;
    }
    default:
    {
        kvsMessageError("Not supported data type.");
        break;
    }
    }

    const size_t width = volume->resolution().x();
    const size_t height = volume->resolution().y();
    const size_t depth = volume->resolution().z();
    m_volume_texture.setPixelFormat( data_format, GL_ALPHA, data_type );
    m_volume_texture.setWrapS( GL_CLAMP_TO_BORDER );
    m_volume_texture.setWrapT( GL_CLAMP_TO_BORDER );
    m_volume_texture.setWrapR( GL_CLAMP_TO_BORDER );
    m_volume_texture.setMagFilter( GL_LINEAR );
    m_volume_texture.setMinFilter( GL_LINEAR );
    m_volume_texture.create( width, height, depth, data_value.data() );
}

void SSAOStochasticUniformGridRenderer::Engine::update_volume_texture(
    const kvs::StructuredVolumeObject* volume )
{
    m_volume_texture.release();
    this->create_volume_texture( volume );
}

/*===========================================================================*/
/**
 *  @brief  Creates trasfer function texture.
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::create_transfer_function_texture()
{
    const size_t width = m_transfer_function.resolution();
    const kvs::ValueArray<kvs::Real32> table = m_transfer_function.table();
    m_transfer_function_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_transfer_function_texture.setMagFilter( GL_LINEAR );
    m_transfer_function_texture.setMinFilter( GL_LINEAR );
    m_transfer_function_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT  );
    m_transfer_function_texture.create( width, table.data() );
    m_transfer_function_changed = false;
}

/*===========================================================================*/
/**
 *  @brief  Creates bounding box buffer.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::create_bounding_cube_buffer(
    const kvs::StructuredVolumeObject* volume )
{
    /* Index number of the bounding cube.
     *
     *       4 ------------ 5
     *     / |            / |
     *    /  |           /  |
     *   7--------------6   |
     *   |   |          |   |
     *   |   0 ---------|-- 1
     *   |  /           |  /
     *   | /            | /
     *   3 ------------ 2
     *
     */
    const kvs::Vector3ui min( 0, 0, 0 );
    const kvs::Vector3ui max( volume->resolution() - kvs::Vector3ui( 1, 1, 1 ) );
    const size_t nelements = 72; // = 4 vertices x 3 dimensions x 6 faces

    const float minx = static_cast<float>( min.x() );
    const float miny = static_cast<float>( min.y() );
    const float minz = static_cast<float>( min.z() );
    const float maxx = static_cast<float>( max.x() );
    const float maxy = static_cast<float>( max.y() );
    const float maxz = static_cast<float>( max.z() );

    const float coords[ nelements ] = {
        minx, miny, minz, // 0
        maxx, miny, minz, // 1
        maxx, miny, maxz, // 2
        minx, miny, maxz, // 3

        minx, maxy, maxz, // 7
        maxx, maxy, maxz, // 6
        maxx, maxy, minz, // 5
        minx, maxy, minz, // 4

        minx, maxy, minz, // 4
        maxx, maxy, minz, // 5
        maxx, miny, minz, // 1
        minx, miny, minz, // 0

        maxx, maxy, minz, // 5
        maxx, maxy, maxz, // 6
        maxx, miny, maxz, // 2
        maxx, miny, minz, // 1

        maxx, maxy, maxz, // 6
        minx, maxy, maxz, // 7
        minx, miny, maxz, // 3
        maxx, miny, maxz, // 2

        minx, miny, minz, // 0
        minx, miny, maxz, // 3
        minx, maxy, maxz, // 7
        minx, maxy, minz  // 4
    };

    kvs::VertexBufferObjectManager::VertexBuffer vertex_array;
    vertex_array.type = GL_FLOAT;
    vertex_array.size = sizeof( float ) * nelements;
    vertex_array.dim = 3;
    vertex_array.pointer = coords;
    m_bounding_cube_buffer.setVertexArray( vertex_array );
    m_bounding_cube_buffer.create();
}

void SSAOStochasticUniformGridRenderer::Engine::update_bounding_cube_buffer(
    const kvs::StructuredVolumeObject* volume )
{
    m_bounding_cube_buffer.release();
    this->create_bounding_cube_buffer( volume );
}

/*===========================================================================*/
/**
 *  @brief  Creates framebuffer.
 *  @param  width [in] window width
 *  @param  height [in] window height
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::create_framebuffer(
    const size_t width,
    const size_t height )
{
    m_entry_texture.setWrapS( GL_CLAMP_TO_BORDER );
    m_entry_texture.setWrapT( GL_CLAMP_TO_BORDER );
    m_entry_texture.setMagFilter( GL_LINEAR );
    m_entry_texture.setMinFilter( GL_LINEAR );
    m_entry_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_entry_texture.create( width, height );

    m_exit_texture.setWrapS( GL_CLAMP_TO_BORDER );
    m_exit_texture.setWrapT( GL_CLAMP_TO_BORDER );
    m_exit_texture.setMagFilter( GL_LINEAR );
    m_exit_texture.setMinFilter( GL_LINEAR );
    m_exit_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT  );
    m_exit_texture.create( width, height );

    m_entry_exit_framebuffer.create();
    m_entry_exit_framebuffer.attachColorTexture( m_exit_texture, 0 );
    m_entry_exit_framebuffer.attachColorTexture( m_entry_texture, 1 );

    auto& shader = m_ao_buffer.geometryPassShader();
    shader.bind();
    shader.setUniform( "width", static_cast<GLfloat>( width ) );
    shader.setUniform( "height", static_cast<GLfloat>( height ) );
    shader.unbind();

    m_ao_buffer.createFramebuffer( width, height );
}

/*===========================================================================*/
/**
 *  @brief  Updates framebuffer
 *  @param  width [in] window width
 *  @param  height [in] window height
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::update_framebuffer(
    const size_t width,
    const size_t height )
{
    m_entry_texture.release();
    m_entry_texture.create( width, height );

    m_exit_texture.release();
    m_exit_texture.create( width, height );

    m_entry_exit_framebuffer.attachColorTexture( m_exit_texture, 0 );
    m_entry_exit_framebuffer.attachColorTexture( m_entry_texture, 1 );

    auto& shader = m_ao_buffer.geometryPassShader();
    shader.bind();
    shader.setUniform( "width", static_cast<GLfloat>( width ) );
    shader.setUniform( "height", static_cast<GLfloat>( height ) );
    shader.unbind();

    m_ao_buffer.updateFramebuffer( width, height );
}

/*===========================================================================*/
/**
 *  @brief  Draws bounding box buffer.
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::draw_bounding_cube_buffer()
{
    kvs::VertexBufferObjectManager::Binder binder( m_bounding_cube_buffer );
    m_bounding_cube_buffer.drawArrays( GL_QUADS, 0, 72 );
}

/*===========================================================================*/
/**
 *  @brief  Draws quad to run shaders.
 */
/*===========================================================================*/
void SSAOStochasticUniformGridRenderer::Engine::draw_quad()
{
    kvs::OpenGL::Disable( GL_LIGHTING );

    kvs::OpenGL::WithPushedMatrix p1( GL_MODELVIEW );
    p1.loadIdentity();
    {
        kvs::OpenGL::WithPushedMatrix p2( GL_PROJECTION );
        p2.loadIdentity();
        {
            kvs::OpenGL::SetOrtho( 0, 1, 0, 1, -1, 1 );
            kvs::OpenGL::Begin( GL_QUADS );
            kvs::OpenGL::Color( kvs::Vec3( 1.0, 1.0, 1.0 ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 1 ), kvs::Vec2( 1, 1 ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 1 ), kvs::Vec2( 0, 1 ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0, 0 ), kvs::Vec2( 0, 0 ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1, 0 ), kvs::Vec2( 1, 0 ) );
            kvs::OpenGL::End();
        }
    }
}

void SSAOStochasticUniformGridRenderer::Engine::draw_buffer_object(
    const kvs::StructuredVolumeObject* volume )
{
    kvs::Texture::Binder unit0( m_volume_texture, 0 );
    kvs::Texture::Binder unit1( m_exit_texture, 1 );
    kvs::Texture::Binder unit2( m_entry_texture, 2 );
    kvs::Texture::Binder unit3( m_transfer_function_texture, 3 );
    kvs::Texture::Binder unit4( randomTexture(), 4 );

    if ( isEnabledShading() ) kvs::OpenGL::Enable( GL_LIGHTING );
    else kvs::OpenGL::Disable( GL_LIGHTING );
    kvs::OpenGL::Disable( GL_CULL_FACE );
    kvs::OpenGL::WithEnabled d( GL_DEPTH_TEST );

    const size_t size = randomTextureSize();
    const int count = repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );
    auto& shader = m_ao_buffer.geometryPassShader();
    shader.setUniform( "random_offset", random_offset );

    this->draw_quad();
}

} // end of namespace AmbientOcclusionRendering
