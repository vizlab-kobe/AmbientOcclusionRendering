/*****************************************************************************/
/**
 *  @file   StochasticTetrahedraRenderer.cpp
 *  @author Naohisa Sakamoto
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

} // end of namespace


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
 *  @brief  Sets edge factor.
 *  @param  factor [in] edge factor
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setEdgeFactor( const float factor )
{
    static_cast<Engine&>( engine() ).setEdgeFactor( factor );
}

/*===========================================================================*/
/**
 *  @brief  Sets a transfer function.
 *  @param  transfer_function [in] transfer function
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setTransferFunction(
    const kvs::TransferFunction& transfer_function )
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

/*===========================================================================*/
/**
 *  @brief  Sets radius of sampling sphere.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setSamplingSphereRadius( const float radius )
{
    static_cast<Engine&>( engine() ).setSamplingSphereRadius( radius );
}

/*===========================================================================*/
/**
 *  @brief  Sets number of sampling points
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::setNumberOfSamplingPoints( const size_t nsamples )
{
    static_cast<Engine&>( engine() ).setNumberOfSamplingPoints( nsamples );
}

/*===========================================================================*/
/**
 *  @brief  Returns transfer function.
 *  @return transfer function
 */
/*===========================================================================*/
const kvs::TransferFunction& SSAOStochasticTetrahedraRenderer::transferFunction() const
{
    return static_cast<const Engine&>( engine() ).transferFunction();
}

/*===========================================================================*/
/**
 *  @brief  Returns sampling step.
 *  @return sampling step
 */
/*===========================================================================*/
float SSAOStochasticTetrahedraRenderer::samplingStep() const
{
    return static_cast<const Engine&>( engine() ).samplingStep();
}

/*===========================================================================*/
/**
 *  @brief  Returns radius of sampling sphere.
 *  @return radius of sampling sphere
 */
/*===========================================================================*/
kvs::Real32 SSAOStochasticTetrahedraRenderer::samplingSphereRadius() const
{
    return static_cast<const Engine&>( engine() ).samplingSphereRadius();
}

/*===========================================================================*/
/**
 *  @brief  Returns number of sampling points.
 *  @return number of sampling points
 */
/*===========================================================================*/
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
    m_edge_factor( 0.0f ),
    m_transfer_function_changed( true ),
    m_sampling_step( 1.0f ),
    m_maxT( 0.0f ),
    m_ao_buffer( this )
{
    m_ao_buffer.setGeometryPassShaderFiles(
        "SSAO_SR_tetrahedra_geom_pass.vert",
        "SSAO_SR_tetrahedra_geom_pass.geom",
        "SSAO_SR_tetrahedra_geom_pass.frag" );

    m_ao_buffer.setOcclusionPassShaderFiles(
        "SSAO_occl_pass.vert",
        "SSAO_occl_pass.frag" );
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::release()
{
    // Release transfer function resources
    m_preintegration_texture.release();
    m_decomposition_texture.release();
    m_transfer_function_texture.release();
    m_T_texture.release();
    m_inv_T_texture.release();
    m_transfer_function_changed = true;

    // Release buffer object resources
    m_vbo_manager.release();

    // Release AO buffer resources
    m_ao_buffer.release();
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
    auto* volume = kvs::UnstructuredVolumeObject::DownCast( object );

    BaseClass::attachObject( object );
    BaseClass::createRandomTexture();

    // Create shader program
    m_ao_buffer.createShaderProgram( BaseClass::shader(), BaseClass::isShadingEnabled() );

    // Create framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    m_ao_buffer.createFramebuffer( framebuffer_width, framebuffer_height );

    // Create buffer object
    this->create_buffer_object( volume );

    // Create transfer function textures
    this->create_preintegration_texture();
    this->create_decomposition_texture();
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
    // Update shader program
    m_ao_buffer.updateShaderProgram( BaseClass::shader(), BaseClass::isShadingEnabled() );

    // Update framebuffer
    const float dpr = camera->devicePixelRatio();
    const size_t framebuffer_width = static_cast<size_t>( camera->windowWidth() * dpr );
    const size_t framebuffer_height = static_cast<size_t>( camera->windowHeight() * dpr );
    m_ao_buffer.updateFramebuffer( framebuffer_width, framebuffer_height );

    // Update buffer object
    this->update_buffer_object( kvs::UnstructuredVolumeObject::DownCast( object ) );

    m_preintegration_texture.release();
    m_transfer_function_texture.release();
    m_T_texture.release();
    m_inv_T_texture.release();
    this->create_preintegration_texture();
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
    // Setup shader program
    m_ao_buffer.setupShaderProgram( this->shader() );

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

    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.bind();
    geom_pass.setUniform( "ModelViewProjectionMatrixInverse", PM_inverse );
    geom_pass.setUniform( "maxT", m_maxT );
    geom_pass.setUniform( "sampling_step_inv", 1.0f / m_sampling_step );
    geom_pass.setUniform( "delta", 0.5f / m_transfer_function.resolution() );
    geom_pass.setUniform( "edge_factor", m_edge_factor );
    geom_pass.unbind();
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
    m_ao_buffer.bind();
    this->draw_buffer_object( kvs::UnstructuredVolumeObject::DownCast( object ) );
    m_ao_buffer.unbind();
    m_ao_buffer.draw();
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
    auto& shader = m_ao_buffer.geometryPassShader();
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

/*===========================================================================*/
/**
 *  @brief  Creates buffer object.
 *  @param  volume [in] pointer to the volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::create_buffer_object(
    const kvs::UnstructuredVolumeObject* volume )
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

    const auto indices = ::RandomIndices( volume, randomTextureSize() );
    const auto values = ::NormalizedValues( volume );
    const auto coords = volume->coords();
    const auto normals = ::VertexNormals( volume );
    auto& geom_pass = m_ao_buffer.geometryPassShader();
    m_vbo_manager.setVertexAttribArray( indices, geom_pass.attributeLocation( "random_index" ), 2 );
    m_vbo_manager.setVertexAttribArray( values, geom_pass.attributeLocation( "value" ), 1 );
    m_vbo_manager.setVertexArray( coords, 3 );
    m_vbo_manager.setNormalArray( normals );
    m_vbo_manager.setIndexArray( volume->connections() );
    m_vbo_manager.create();
}

/*===========================================================================*/
/**
 *  @brief  Updates buffer object.
 *  @param  volume [in] pointer to the unstructured volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::update_buffer_object( const kvs::UnstructuredVolumeObject* volume )
{
    m_vbo_manager.release();
    this->create_buffer_object( volume );
}

/*===========================================================================*/
/**
 *  @brief  Draws buffer object.
 *  @param  volume [in] pointer to the unstructured volume object
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::draw_buffer_object(
    const kvs::UnstructuredVolumeObject* volume )
{
    kvs::OpenGL::WithEnabled d( GL_DEPTH_TEST );

    // Random factors
    const size_t size = BaseClass::randomTextureSize();
    const int count = BaseClass::repetitionCount() * ::RandomNumber();
    const float offset_x = static_cast<float>( ( count ) % size );
    const float offset_y = static_cast<float>( ( count / size ) % size );
    const kvs::Vec2 random_offset( offset_x, offset_y );

    // Updates variables in geom pass shader
    auto& geom_pass = m_ao_buffer.geometryPassShader();
    geom_pass.setUniform( "random_offset", random_offset );
    geom_pass.setUniform( "random_texture", 0 );
    geom_pass.setUniform( "preintegration_texture", 1 );
    geom_pass.setUniform( "decomposition_texture", 2 );
    geom_pass.setUniform( "transfer_function_texture", 3 );
    geom_pass.setUniform( "T_texture", 4 );
    geom_pass.setUniform( "invT_texture", 5 );

    // Draw buffer object
    kvs::Texture::Binder bind0( randomTexture(), 0 );
    kvs::Texture::Binder bind1( m_preintegration_texture, 1 );
    kvs::Texture::Binder bind2( m_decomposition_texture, 2 );
    kvs::Texture::Binder bind3( m_transfer_function_texture, 3 );
    kvs::Texture::Binder bind4( m_T_texture, 4 );
    kvs::Texture::Binder bind5( m_inv_T_texture, 5 );

    kvs::VertexBufferObjectManager::Binder bind( m_vbo_manager );
    const size_t ncells = volume->numberOfCells();
    m_vbo_manager.drawElements( GL_LINES_ADJACENCY_EXT, 4 * ncells );
}

/*===========================================================================*/
/**
 *  @brief  Sets geometry pass shader files.
 *  @param  vert_file [in] vertex shader file
 *  @param  geom_file [in] geometry shader file
 *  @param  frag_file [in] fragment shader file
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::AmbientOcclusionBuffer::setGeometryPassShaderFiles(
    const std::string& vert_file,
    const std::string& geom_file,
    const std::string& frag_file )
{
    BaseClass::setGeometryPassShaderFiles( vert_file, frag_file );
    m_geom_pass_shader_geom_file = geom_file;
}

/*===========================================================================*/
/**
 *  @brief  Creates shader program.
 *  @param  shading_model [in] shading model
 *  @param  shading_enabled [in] shading will be enabled if true
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::AmbientOcclusionBuffer::createShaderProgram(
    const kvs::Shader::ShadingModel& shading_model,
    const bool shading_enabled )
{
    // Build SSAO shader for geometry-pass (1st pass).
    {
        kvs::ShaderSource vert( BaseClass::geometryPassVertexShaderFile() );
        kvs::ShaderSource geom( this->geometryPassGeometryShaderFile() );
        kvs::ShaderSource frag( BaseClass::geometryPassFragmentShaderFile() );

        if ( m_engine->depthTexture().isCreated() )
        {
            vert.define("ENABLE_EXACT_DEPTH_TESTING");
            geom.define("ENABLE_EXACT_DEPTH_TESTING");
            frag.define("ENABLE_EXACT_DEPTH_TESTING");
        }

        // Build
        auto& geom_pass = BaseClass::geometryPassShader();
        geom_pass.setGeometryInputType( GL_LINES_ADJACENCY_EXT );
        geom_pass.setGeometryOutputType( GL_TRIANGLE_STRIP );
        geom_pass.setGeometryOutputVertices( 4 * 3 );
        geom_pass.build( vert, geom, frag );
    }

    // Build SSAO shader for occlusion-pass (2nd pass).
    {
        kvs::ShaderSource vert( BaseClass::occlusionPassVertexShaderFile() );
        kvs::ShaderSource frag( BaseClass::occlusionPassFragmentShaderFile() );
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

            const size_t nsamples = BaseClass::numberOfSamplingPoints();
            frag.define( "NUMBER_OF_SAMPLING_POINTS " + kvs::String::ToString( nsamples ) );
        }

        // Build
        auto& occl_pass = BaseClass::occlusionPassShader();
        occl_pass.build( vert, frag );

        // Generate points in sampling sphere
        const size_t nsamples = BaseClass::numberOfSamplingPoints();
        const float radius = BaseClass::samplingSphereRadius();
        const size_t dim = 3;
        const auto sampling_points = BaseClass::generatePoints( radius, nsamples );
        occl_pass.bind();
        occl_pass.setUniform( "sampling_points", sampling_points, dim );
        occl_pass.unbind();
    }
}

/*===========================================================================*/
/**
 *  @brief  Updates shader program.
 *  @param  shading_model [in] shading model
 *  @param  shading_enabled [in] shading will be enabled if true
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::AmbientOcclusionBuffer::updateShaderProgram(
    const kvs::Shader::ShadingModel& shading_model,
    const bool shading_enabled )
{
    BaseClass::geometryPassShader().release();
    BaseClass::occlusionPassShader().release();
    this->createShaderProgram( shading_model, shading_enabled );
}

/*===========================================================================*/
/**
 *  @brief  Setups shader program.
 *  @param  shading_model [in] shading model
 *  @param  shading_enabled [in] shading will be enabled if true
 */
/*===========================================================================*/
void SSAOStochasticTetrahedraRenderer::Engine::AmbientOcclusionBuffer::setupShaderProgram(
    const kvs::Shader::ShadingModel& shading_model )
{
    BaseClass::setupShaderProgram( shading_model );

    // Geometry pass shader
    auto& geom_pass = geometryPassShader();
    geom_pass.bind();
    geom_pass.setUniform( "random_texture_size_inv", 1.0f / m_engine->randomTextureSize() );
    geom_pass.unbind();
}

} // end of namespace AmbientOcclusionRendering
