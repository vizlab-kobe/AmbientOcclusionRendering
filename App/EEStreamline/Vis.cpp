#include "Vis.h"
#include <kvs/ValueArray>
#include <kvs/SmartPointer>
#include <StochasticStreamline/Lib/Streamline.h>
#include "Streamline.h"


namespace local
{

kvs::StructuredVolumeObject* Vis::import( const bool cache ) const
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( m_input.filename );
    if ( cache ) { m_cached_volume = volume; }

    kvs::ValueArray<float> values = volume->values().asValueArray<float>();
    for ( size_t i = 0; i < values.size(); i++ ) { values[i] *= m_input.scale; }
    volume->setValues( values );
    volume->updateMinMaxValues();
    return volume;
}

kvs::LineObject* Vis::streamline( const kvs::StructuredVolumeObject* volume ) const
{
    if ( !volume ) { volume = m_cached_volume; }
    if ( !volume ) { return NULL; }

    kvs::SharedPointer<kvs::PointObject> seeds( this->generate_seed_points() );
//    typedef StochasticStreamline::Streamline Mapper;
    typedef local::Streamline Mapper;
    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( seeds.get() );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    mapper->setTransferFunction( m_input.tfunc );
    return mapper->exec( volume );
}

kvs::RendererBase* Vis::renderer() const
{
    if ( m_input.ssao )
    {
        SSAORenderer* renderer = new SSAORenderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( m_input.tfunc );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( m_input.repeats );
        renderer->setEnabledLODControl( m_input.lod );
        renderer->enableShading();
        renderer->setSamplingSphereRadius( m_input.radius );
        renderer->setNumberOfSamplingPoints( m_input.points );
        renderer->setShaderMode( m_input.shader );
        return renderer;
    }
    else
    {
        NoSSAORenderer* renderer = new NoSSAORenderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( m_input.tfunc );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( m_input.repeats );
        renderer->setEnabledLODControl( m_input.lod );
        renderer->enableShading();
        return renderer;
    }
}

kvs::PointObject* Vis::generate_seed_points() const
{
    std::vector<kvs::Real32> v;
    for ( int k = m_input.min_coord.z(); k < m_input.max_coord.z(); k += m_input.stride.z() )
    {
        for ( int j = m_input.min_coord.y(); j < m_input.max_coord.y(); j += m_input.stride.y() )
        {
            for ( int i = m_input.min_coord.x(); i < m_input.max_coord.x(); i += m_input.stride.x() )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::PointObject* seeds = new kvs::PointObject;
    seeds->setCoords( kvs::ValueArray<kvs::Real32>( v ) );
    return seeds;
}

} // end of namespace local
