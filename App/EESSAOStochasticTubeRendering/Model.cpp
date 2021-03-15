#include "Model.h"
#include <kvs/ValueArray>
#include <kvs/SmartPointer>
#include <StochasticStreamline/Lib/Streamline.h>
#include "Streamline.h"


namespace local
{

Model::Model( local::Input& input ):
    Input( input ),
    m_cached_volume( nullptr )
{
    input.print( std::cout << "Input Variables" << std::endl, kvs::Indent( 4 ) );
}

kvs::StructuredVolumeObject* Model::import( const bool cache ) const
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( this->filename() );
    if ( cache ) { m_cached_volume = volume; }

    kvs::ValueArray<float> values = volume->values().asValueArray<float>();
    for ( size_t i = 0; i < values.size(); i++ ) { values[i] *= this->scale(); }
    volume->setValues( values );
    volume->updateMinMaxValues();
    return volume;
}

kvs::LineObject* Model::streamline( const kvs::StructuredVolumeObject* volume ) const
{
    if ( !volume ) { volume = m_cached_volume; }
    if ( !volume ) { return NULL; }

    kvs::SharedPointer<kvs::PointObject> seeds( this->generate_seed_points() );
    //typedef StochasticStreamline::Streamline Mapper;
    typedef local::Streamline Mapper;
    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( seeds.get() );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    mapper->setTransferFunction( this->transferFunction() );
    return mapper->exec( volume );
}

kvs::RendererBase* Model::renderer() const
{
    if ( this->isSSAOEnabled() )
    {
        SSAORenderer* renderer = new SSAORenderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( this->transferFunction() );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( this->repeats() );
        renderer->setEnabledLODControl( this->isLODEnabled() );
        renderer->enableShading();
        renderer->setSamplingSphereRadius( this->radius() );
        renderer->setNumberOfSamplingPoints( this->points() );
        renderer->setEdgeFactor( this->edge() );
        return renderer;
    }
    else
    {
        NoSSAORenderer* renderer = new NoSSAORenderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( this->transferFunction() );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( this->repeats() );
        renderer->setEnabledLODControl( this->isLODEnabled() );
        renderer->enableShading();
        return renderer;
    }
}

kvs::PointObject* Model::generate_seed_points() const
{
    std::vector<kvs::Real32> v;
    for ( int k = this->minCoord().z(); k < this->maxCoord().z(); k += this->stride().z() )
    {
        for ( int j = this->minCoord().y(); j < this->maxCoord().y(); j += this->stride().y() )
        {
            for ( int i = this->minCoord().x(); i < this->maxCoord().x(); i += this->stride().x() )
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
