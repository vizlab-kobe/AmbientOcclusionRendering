/*****************************************************************************/
/**
 *  @file   StochasticTetrahedraRenderer.cpp
 *  @author Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright (c) Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id$
 */
/*----------------------------------------------------------------------------
 *
 * References:
 * [1] Naohisa Sakamoto, Koji Koyamada, "A Stochastic Approach for Rendering
 *     Multiple Irregular Volumes", In Proc. of IEEE Pacific Visualization
 *     2014 (VisNotes), pp.272-276, 2014.3.
 */
/*****************************************************************************/
#include "SSAOStochasticTetrahedraRenderer.h"
#include "SSAOPointSampling.h"
#include <cmath>
#include <kvs/OpenGL>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/Assert>
#include <kvs/Message>
#include <kvs/Type>
#include <kvs/Xorshift128>
#include <kvs/TetrahedralCell>
#include <kvs/ProjectedTetrahedraTable>
#include <kvs/PreIntegrationTable2D>


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
 *  @brief  Returns random index array.
 *  @param  volume [in] pointer to the volume object
 *  @param  size [in] random texture size
 */
/*===========================================================================*/
kvs::ValueArray<kvs::UInt16> RandomIndices( const kvs::UnstructuredVolumeObject* volume, const size_t size )
{
    const size_t nnodes = volume->numberOfNodes();
    kvs::ValueArray<kvs::UInt16> indices( nnodes * 2 );
    for ( size_t i = 0; i < nnodes; i++ )
    {
        const unsigned int count = i * RandomNumber();
        indices[ 2 * i + 0 ] = static_cast<kvs::UInt16>( ( count ) % size );
        indices[ 2 * i + 1 ] = static_cast<kvs::UInt16>( ( count / size ) % size );
    }

    return indices;
}

/*===========================================================================*/
/**
 *  @brief  Returns normalized value array for type of value array of the input volume.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
template <typename T>
kvs::ValueArray<kvs::Real32> NormalizedValuesForType( const kvs::UnstructuredVolumeObject* volume )
{
    if ( !volume->hasMinMaxValues() ) volume->updateMinMaxValues();
    const kvs::Real64 min_value = volume->minValue();
    const kvs::Real64 max_value = volume->maxValue();
    const kvs::Real64 normalized_factor = 1.0 / ( max_value - min_value );

    const T* src = static_cast<const T*>( volume->values().data() );
    const size_t nvalues = volume->values().size();
    kvs::ValueArray<kvs::Real32> dst( nvalues );
    for ( size_t i = 0; i < nvalues; i++ )
    {
        dst[i] = static_cast<kvs::Real32>( ( src[i] - min_value ) * normalized_factor );
    }

    return dst;
}

/*===========================================================================*/
/**
 *  @brief  Returns normalized value array for the input volume.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::Real32> NormalizedValues( const kvs::UnstructuredVolumeObject* volume )
{
    switch ( volume->values().typeID() )
    {
    case kvs::Type::TypeReal32: return NormalizedValuesForType<kvs::Real32>( volume );
    case kvs::Type::TypeReal64: return NormalizedValuesForType<kvs::Real64>( volume );
    default: break;
    }

    kvsMessageError("Not supported data type.");
    return kvs::ValueArray<kvs::Real32>();
}

/*===========================================================================*/
/**
 *  @brief  Returns normal vector array for type of value array of the input volume.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::Real32> VertexNormalsForType( const kvs::UnstructuredVolumeObject* volume )
{
    const size_t nnodes = volume->numberOfNodes();
    const size_t ncells = volume->numberOfCells();
    kvs::TetrahedralCell cell( volume );
    kvs::ValueArray<kvs::Int32> counter( nnodes );
    kvs::ValueArray<kvs::Real32> normals( nnodes * 3 );
    counter.fill(0);
    normals.fill(0);
    for ( size_t i = 0; i < ncells; i++ )
    {
        cell.bindCell( i );
        const kvs::Vec3 g = -cell.gradientVector();
        const kvs::UInt32 index0 = volume->connections()[ 4 * i + 0 ];
        const kvs::UInt32 index1 = volume->connections()[ 4 * i + 1 ];
        const kvs::UInt32 index2 = volume->connections()[ 4 * i + 2 ];
        const kvs::UInt32 index3 = volume->connections()[ 4 * i + 3 ];

        counter[ index0 ]++;
        counter[ index1 ]++;
        counter[ index2 ]++;
        counter[ index3 ]++;

        normals[ 3 * index0 + 0 ] += g.x();
        normals[ 3 * index0 + 1 ] += g.y();
        normals[ 3 * index0 + 2 ] += g.z();

        normals[ 3 * index1 + 0 ] += g.x();
        normals[ 3 * index1 + 1 ] += g.y();
        normals[ 3 * index1 + 2 ] += g.z();

        normals[ 3 * index2 + 0 ] += g.x();
        normals[ 3 * index2 + 1 ] += g.y();
        normals[ 3 * index2 + 2 ] += g.z();

        normals[ 3 * index3 + 0 ] += g.x();
        normals[ 3 * index3 + 1 ] += g.y();
        normals[ 3 * index3 + 2 ] += g.z();
    }

    for ( size_t i = 0; i < nnodes; i++ )
    {
        const kvs::Real32 c = static_cast<kvs::Real32>( counter[i] );
        const kvs::Vec3 v( normals.data() + 3 * i );
        const kvs::Vec3 n = ( v / c ).normalized();
        normals[ 3 * i + 0 ] = n.x();
        normals[ 3 * i + 1 ] = n.y();
        normals[ 3 * i + 2 ] = n.z();
    }

    return normals;
}

/*===========================================================================*/
/**
 *  @brief  Returns normal vector array for type of value array of the input volume.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
kvs::ValueArray<kvs::Real32> VertexNormals( const kvs::UnstructuredVolumeObject* volume )
{
    switch ( volume->values().typeID() )
    {
    case kvs::Type::TypeReal32:
    case kvs::Type::TypeReal64:
        return VertexNormalsForType( volume );
    default: break;
    }

    kvsMessageError("Not supported data type.");
    return kvs::ValueArray<kvs::Real32>();
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

}


namespace AmbientOcclusionRendering
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new StochasticTetrahedraRenderer class.
 */
/*===========================================================================*/
SSAOStochasticTetrahedraRenderer::SSAOStochasticTetrahedraRenderer():
    StochasticRendererBase( new Engine() )
{
}

/*===========================================================================*/
/**
 *  @brief  Sets a transfer function.
 *  @param  transfer_function [in] transfer function
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setTransferFunction( const kvs::TransferFunction& transfer_function )
{
    static_cast<Engine&>( engine() ).setTransferFunction( transfer_function );
}

/*===========================================================================*/
/**
 *  @brief  Sets a sampling step.
 *  @param  sampling_step [in] sampling step
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setSamplingStep( const float sampling_step )
{
    static_cast<Engine&>( engine() ).setSamplingStep( sampling_step );
}

const kvs::TransferFunction& SSAOStochasticTetrahedraRenderer::transferFunction() const
{
    return static_cast<const Engine&>( engine() ).transferFunction();
}

float SSAOStochasticTetrahedraRenderer::samplingStep() const
{
    return static_cast<const Engine&>( engine() ).samplingStep();
}

void SSAOStochasticTetrahedraRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

void SSAOStochasticTetrahedraRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

kvs::Real32 SSAOStochasticTetrahedraRenderer::samplingSphereRadius() const
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

size_t SSAOStochasticTetrahedraRenderer::numberOfSamplingPoints() const
{
    return static_cast<const Engine&>( engine() ).numberOfSamplingPoints();
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
SSAOStochasticTetrahedraRenderer::Engine::Engine():
    m_transfer_function_changed( true ),
    m_sampling_step( 1.0f ),
    m_maxT( 0.0f ),
    m_drawable( this )
{
    m_drawable.setGeometryPassShaderFiles( "SSAO_SR_tetrahedra_geom_pass.vert", "SSAO_SR_tetrahedra_geom_pass.geom", "SSAO_SR_tetrahedra_geom_pass.frag" );
    m_drawable.setOcclusionPassShaderFiles( "SSAO_occl_pass.vert", "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::release()
{
//    m_shader_geom_pass.release();
//    m_shader_occl_pass.release();
    m_vbo_manager.release();
    m_preintegration_texture.release();
    m_decomposition_texture.release();
    m_transfer_function_texture.release();
    m_T_texture.release();
    m_inv_T_texture.release();
    m_transfer_function_changed = true;
//    m_framebuffer.release();
//    m_color_texture.release();
//    m_position_texture.release();
//    m_normal_texture.release();
//    m_depth_texture.release();
    m_drawable.releaseResources();
}

/*===========================================================================*/
/**
 *  @brief  Create shaders and buffer objects.
 *  @param  object [in] pointer to the polygon object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    kvs::UnstructuredVolumeObject* volume = kvs::UnstructuredVolumeObject::DownCast( object );

    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );

    attachObject( object );
    createRandomTexture();
//    this->create_shader_program();
    m_drawable.createShaderProgram( this->shader(), this->isEnabledShading() );
    m_drawable.createFramebuffer( framebuffer_width, framebuffer_height );
    this->create_buffer_object( volume );
    this->create_preintegration_texture();
    this->create_decomposition_texture();
//    this->create_framebuffer( framebuffer_width, framebuffer_height );
//    this->create_sampling_points();
}

/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  polygon [in] pointer to the polygon object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::update(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
//    this->update_framebuffer( framebuffer_width, framebuffer_height );
    m_drawable.updateFramebuffer( framebuffer_width, framebuffer_height );
}

/*===========================================================================*/
/**
 *  @brief  Set up.
 *  @param  polygon [in] pointer to the polygon object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::setup(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    if ( m_transfer_function_changed )
    {
        // Re-create pre-integration table.
        m_preintegration_texture.release();
        m_transfer_function_texture.release();
        m_T_texture.release();
        m_inv_T_texture.release();
        this->create_preintegration_texture();
    }

    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 PM = kvs::OpenGL::ProjectionMatrix() * M;
    const kvs::Mat4 PM_inverse = PM.inverted();
    const kvs::Mat3 N = kvs::Mat3( M[0].xyz(), M[1].xyz(), M[2].xyz() );
/*
    m_shader_geom_pass.bind();
    m_shader_geom_pass.setUniform( "ModelViewMatrix", M );
    m_shader_geom_pass.setUniform( "ModelViewProjectionMatrix", PM );
    m_shader_geom_pass.setUniform( "ModelViewProjectionMatrixInverse", PM_inverse );
    m_shader_geom_pass.setUniform( "NormalMatrix", N );
    m_shader_geom_pass.setUniform( "maxT", m_maxT );
    m_shader_geom_pass.setUniform( "sampling_step_inv", 1.0f / m_sampling_step );
    m_shader_geom_pass.setUniform( "delta", 0.5f / m_transfer_function.resolution() );
    m_shader_geom_pass.unbind();
*/
    auto& shader = m_drawable.geometryPassShader();
    shader.bind();
    shader.setUniform( "ModelViewMatrix", M );
    shader.setUniform( "ModelViewProjectionMatrix", PM );
    shader.setUniform( "ModelViewProjectionMatrixInverse", PM_inverse );
    shader.setUniform( "NormalMatrix", N );
    shader.setUniform( "maxT", m_maxT );
    shader.setUniform( "sampling_step_inv", 1.0f / m_sampling_step );
    shader.setUniform( "delta", 0.5f / m_transfer_function.resolution() );
    shader.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Draw an ensemble.
 *  @param  object [in] pointer to the unstructured volume object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::draw(
    kvs::ObjectBase* object,
    kvs::Camera* camera,
    kvs::Light* light )
{
    this->render_geometry_pass( kvs::UnstructuredVolumeObject::DownCast( object ) );
    this->render_occlusion_pass();
}

/*===========================================================================*/
/**
 *  @brief  Creates shader program.
 */
/*===========================================================================*/
/*
void SSAOStochasticTetrahedraRenderer::Engine::create_shader_program()
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( "SSAO_SR_tetrahedra_geom_pass.vert" );
        kvs::ShaderSource geom( "SSAO_SR_tetrahedra_geom_pass.geom" );
        kvs::ShaderSource frag( "SSAO_SR_tetrahedra_geom_pass.frag" );

        if ( depthTexture().isCreated() )
        {
            vert.define("ENABLE_EXACT_DEPTH_TESTING");
            geom.define("ENABLE_EXACT_DEPTH_TESTING");
            frag.define("ENABLE_EXACT_DEPTH_TESTING");
        }

        // Parameters for geometry shader.
        m_shader_geom_pass.setGeometryInputType( GL_LINES_ADJACENCY_EXT );
        m_shader_geom_pass.setGeometryOutputType( GL_TRIANGLE_STRIP );
        m_shader_geom_pass.setGeometryOutputVertices( 4 * 3 );

        // Build shaders.
        m_shader_geom_pass.build( vert, geom, frag );
        m_shader_geom_pass.bind();
        m_shader_geom_pass.setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
        m_shader_geom_pass.setUniform( "random_texture", 0 );
        m_shader_geom_pass.setUniform( "preintegration_texture", 1 );
        m_shader_geom_pass.setUniform( "decomposition_texture", 2 );
        m_shader_geom_pass.setUniform( "transfer_function_texture", 3 );
        m_shader_geom_pass.setUniform( "T_texture", 4 );
        m_shader_geom_pass.setUniform( "invT_texture", 5 );
        m_shader_geom_pass.unbind();
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
*/

/*===========================================================================*/
/**
 *  @brief  Create buffer objects.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_buffer_object( const kvs::UnstructuredVolumeObject* volume )
{
    if ( volume->cellType() != kvs::UnstructuredVolumeObject::Tetrahedra )
    {
        kvsMessageError("Not supported cell type.");
        return;
    }

    if ( volume->veclen() != 1 )
    {
        kvsMessageError("Not scalar volume.");
        return;
    }

    const kvs::ValueArray<kvs::UInt16> indices = ::RandomIndices( volume, randomTextureSize() );
    const kvs::ValueArray<kvs::Real32> values = ::NormalizedValues( volume );
    const kvs::ValueArray<kvs::Real32> coords = volume->coords();
    const kvs::ValueArray<kvs::Real32> normals = ::VertexNormals( volume );
//    m_vbo_manager.setVertexAttribArray( indices, m_shader_geom_pass.attributeLocation("random_index"), 2 );
//    m_vbo_manager.setVertexAttribArray( values, m_shader_geom_pass.attributeLocation("value"), 1 );
    m_vbo_manager.setVertexAttribArray( indices, m_drawable.geometryPassShader().attributeLocation( "random_index" ), 2 );
    m_vbo_manager.setVertexAttribArray( values, m_drawable.geometryPassShader().attributeLocation( "value" ), 1 );
    m_vbo_manager.setVertexArray( coords, 3 );
    m_vbo_manager.setNormalArray( normals );
    m_vbo_manager.setIndexArray( volume->connections() );
    m_vbo_manager.create();
}

/*===========================================================================*/
/**
 *  @brief  Creates pre-integration texture.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_preintegration_texture()
{
    kvs::PreIntegrationTable2D table;
    table.setTransferFunction( m_transfer_function );
    table.create();

    int resolution_inverse_texture_size = kvs::Math::Min( 16384, kvs::OpenGL::MaxTextureSize() );
/*
    m_shader_geom_pass.bind();
    m_shader_geom_pass.setUniform( "delta2", 0.5f / resolution_inverse_texture_size );
    m_shader_geom_pass.unbind();
*/
    auto& shader = m_drawable.geometryPassShader();
    shader.bind();
    shader.setUniform( "delta2", 0.5f / resolution_inverse_texture_size );
    shader.unbind();

    kvs::ValueArray<kvs::Real32> T = table.T();
    kvs::ValueArray<kvs::Real32> invT = table.inverseT( resolution_inverse_texture_size );
    size_t resolution = T.size();

    m_maxT = T.back();

    m_T_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_T_texture.setMagFilter( GL_LINEAR );
    m_T_texture.setMinFilter( GL_LINEAR );
    m_T_texture.setPixelFormat( GL_R32F, GL_RED, GL_FLOAT  );
    m_T_texture.create( T.size(), T.data() );

    m_inv_T_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_inv_T_texture.setMagFilter( GL_LINEAR );
    m_inv_T_texture.setMinFilter( GL_LINEAR );
    m_inv_T_texture.setPixelFormat( GL_R32F, GL_RED, GL_FLOAT  );
    m_inv_T_texture.create( invT.size(), invT.data() );

    m_transfer_function_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_transfer_function_texture.setMagFilter( GL_LINEAR );
    m_transfer_function_texture.setMinFilter( GL_LINEAR );
    m_transfer_function_texture.setPixelFormat( GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT );
    m_transfer_function_texture.create( m_transfer_function.resolution(), m_transfer_function.table().data() );

    m_preintegration_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_preintegration_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_preintegration_texture.setMagFilter( GL_LINEAR );
    m_preintegration_texture.setMinFilter( GL_LINEAR );
    m_preintegration_texture.setPixelFormat( GL_R32F, GL_RED, GL_FLOAT );
    m_preintegration_texture.create( resolution, resolution, table.table().data() );

    m_transfer_function_changed = false;
}

/*===========================================================================*/
/**
 *  @brief  Creates decomposition texture.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_decomposition_texture()
{
    kvs::ValueArray<GLubyte> table( 81 * 4 );
    for ( size_t i = 0; i < 81; i++ )
    {
        table[ i * 4 + 0 ] = kvs::ProjectedTetrahedraTable::PatternInfo[i][1] * 32;
        table[ i * 4 + 1 ] = kvs::ProjectedTetrahedraTable::PatternInfo[i][2] * 32;
        table[ i * 4 + 2 ] = kvs::ProjectedTetrahedraTable::PatternInfo[i][3] * 32;
        table[ i * 4 + 3 ] = kvs::ProjectedTetrahedraTable::PatternInfo[i][0] * 32;
    }

    m_decomposition_texture.setWrapS( GL_CLAMP_TO_EDGE );
    m_decomposition_texture.setWrapT( GL_CLAMP_TO_EDGE );
    m_decomposition_texture.setMagFilter( GL_NEAREST );
    m_decomposition_texture.setMinFilter( GL_NEAREST );
    m_decomposition_texture.setPixelFormat( GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE );
    m_decomposition_texture.create( 81, 1, table.data() );
}

/*
void SSAOStochasticTetrahedraRenderer::Engine::create_framebuffer( const size_t width, const size_t height )
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

void SSAOStochasticTetrahedraRenderer::Engine::create_sampling_points()
{
    const size_t nsamples = m_nsamples;
    const float radius = m_sampling_sphere_radius;
    const size_t dim = 3;
    const kvs::ValueArray<GLfloat> sampling_points = AmbientOcclusionRendering::SSAOPointSampling( radius, nsamples );
    m_shader_occl_pass.bind();
    m_shader_occl_pass.setUniform( "sampling_points", sampling_points, dim );
    m_shader_occl_pass.unbind();
}

void SSAOStochasticTetrahedraRenderer::Engine::update_framebuffer( const size_t width, const size_t height )
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

void SSAOStochasticTetrahedraRenderer::Engine::render_geometry_pass(
    const kvs::UnstructuredVolumeObject* volume )
{
//    kvs::FrameBufferObject::GuardedBinder bind0( m_framebuffer );
    kvs::FrameBufferObject::GuardedBinder bind0( m_drawable.framebuffer() );

    // Initialize FBO.
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
    kvs::Texture::Binder bind3( randomTexture(), 0 );
    kvs::Texture::Binder bind4( m_preintegration_texture, 1 );
    kvs::Texture::Binder bind5( m_decomposition_texture, 2 );
    kvs::Texture::Binder bind6( m_transfer_function_texture, 3 );
    kvs::Texture::Binder bind7( m_T_texture, 4 );
    kvs::Texture::Binder bind8( m_inv_T_texture, 5 );
    {
        kvs::OpenGL::WithEnabled d( GL_DEPTH_TEST );

        const size_t size = randomTextureSize();
        const int count = repetitionCount() * ::RandomNumber();
        const float offset_x = static_cast<float>( ( count ) % size );
        const float offset_y = static_cast<float>( ( count / size ) % size );
        const kvs::Vec2 random_offset( offset_x, offset_y );
//        m_shader_geom_pass.setUniform( "random_offset", random_offset );
        m_drawable.geometryPassShader().setUniform( "random_offset", random_offset );

        const size_t ncells = volume->numberOfCells();
        m_vbo_manager.drawElements( GL_LINES_ADJACENCY_EXT, 4 * ncells );
    }
}

void SSAOStochasticTetrahedraRenderer::Engine::render_occlusion_pass()
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

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );
    ::Draw();
    */
    m_drawable.renderOcclusionPass();
}

void SSAOStochasticTetrahedraRenderer::Engine::Drawable::setGeometryPassShaderFiles(
    const std::string& vert_file,
    const std::string& geom_file,
    const std::string& frag_file )
{
    SSAODrawable::setGeometryPassShaderFiles( vert_file, frag_file );
    m_geom_pass_shader_geom_file = geom_file;
}

void SSAOStochasticTetrahedraRenderer::Engine::Drawable::createShaderProgram(
    const kvs::Shader::ShadingModel& shading_model,
    const bool shading_enabled )
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( geometryPassVertexShaderFile() );
        kvs::ShaderSource geom( geometryPassGeometryShaderFile() );
        kvs::ShaderSource frag( geometryPassFragmentShaderFile() );

        if ( m_engine->depthTexture().isCreated() )
        {
            vert.define("ENABLE_EXACT_DEPTH_TESTING");
            geom.define("ENABLE_EXACT_DEPTH_TESTING");
            frag.define("ENABLE_EXACT_DEPTH_TESTING");
        }

        // Parameters for geometry shader.
        auto& shader = geometryPassShader();
        shader.setGeometryInputType( GL_LINES_ADJACENCY_EXT );
        shader.setGeometryOutputType( GL_TRIANGLE_STRIP );
        shader.setGeometryOutputVertices( 4 * 3 );

        // Build shaders.
        shader.build( vert, geom, frag );

        shader.bind();
        shader.setUniform( "random_texture_size_inv", 1.0f / m_engine->randomTextureSize() );
        shader.setUniform( "random_texture", 0 );
        shader.setUniform( "preintegration_texture", 1 );
        shader.setUniform( "decomposition_texture", 2 );
        shader.setUniform( "transfer_function_texture", 3 );
        shader.setUniform( "T_texture", 4 );
        shader.setUniform( "invT_texture", 5 );
        shader.unbind();
    }

    // Build SSAO shader for occlusion-pass (2nd pass).
    {
        kvs::ShaderSource vert( occlusionPassVertexShaderFile() );
        kvs::ShaderSource frag( occlusionPassFragmentShaderFile() );
        if ( shading_enabled )
        {
            switch ( shading_model.type() )
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

            const size_t nsamples = numberOfSamplingPoints();
            frag.define( "NUMBER_OF_SAMPLING_POINTS " + kvs::String::ToString( nsamples ) );
        }

        auto& shader = occlusionPassShader();
        shader.build( vert, frag );

        const size_t nsamples = numberOfSamplingPoints();
        const float radius = samplingSphereRadius();
        const size_t dim = 3;
        const auto sampling_points = AmbientOcclusionRendering::SSAOPointSampling( radius, nsamples );

        shader.bind();
        shader.setUniform( "shading.Ka", shading_model.Ka );
        shader.setUniform( "shading.Kd", shading_model.Kd );
        shader.setUniform( "shading.Ks", shading_model.Ks );
        shader.setUniform( "shading.S",  shading_model.S );
        shader.setUniform( "sampling_points", sampling_points, dim );
        shader.unbind();
    }
}

void SSAOStochasticTetrahedraRenderer::Engine::Drawable::updateShaderProgram(
    const kvs::Shader::ShadingModel& shading_model,
    const bool shading_enabled )
{
    geometryPassShader().release();
    occlusionPassShader().release();
    createShaderProgram( shading_model, shading_enabled );
}

} // end of namespace AmbientOcclusionRendering
