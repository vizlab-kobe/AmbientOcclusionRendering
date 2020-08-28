#pragma once
#include <kvs/Module>
#include <kvs/GridBase>
#include <kvs/CellBase>
#include <kvs/CellLocator>
//#include "StreamlineBase.h"
#include "StreamlineBase.h"


//namespace StochasticStreamline
namespace local
{

/*===========================================================================*/
/**
 *  Streamline class.
 */
/*===========================================================================*/
class Streamline : public StochasticStreamline::StreamlineBase
{
//    kvsModule( StochasticStreamline::Streamline, Mapper );
    kvsModule( local::Streamline, Mapper );
    kvsModuleBaseClass( StochasticStreamline::StreamlineBase );

public:

    class StructuredVolumeInterpolator : public Interpolator
    {
    private:
        kvs::GridBase* m_grid;
    public:
        StructuredVolumeInterpolator( const kvs::StructuredVolumeObject* volume );
        ~StructuredVolumeInterpolator();
        kvs::Vec3 interpolatedValue( const kvs::Vec3& point );
        bool containsInVolume( const kvs::Vec3& point );
    };

    class UnstructuredVolumeInterpolator : public Interpolator
    {
    private:
        kvs::CellBase* m_cell;
        kvs::CellLocator* m_locator;
    public:
        UnstructuredVolumeInterpolator( const kvs::UnstructuredVolumeObject* volume );
        ~UnstructuredVolumeInterpolator();
        kvs::Vec3 interpolatedValue( const kvs::Vec3& point );
        bool containsInVolume( const kvs::Vec3& point );
    };

    class EulerIntegrator : public Integrator
    {
    public:
        kvs::Vec3 next( const kvs::Vec3& point );
    };

    class RungeKutta2ndIntegrator : public Integrator
    {
    public:
        kvs::Vec3 next( const kvs::Vec3& point );
    };

    class RungeKutta4thIntegrator : public Integrator
    {
    public:
        kvs::Vec3 next( const kvs::Vec3& point );
    };

public:

    Streamline() {}

    Streamline(
        const kvs::VolumeObjectBase* volume,
        const kvs::PointObject* seed_points,
        const kvs::TransferFunction& transfer_function );

    BaseClass::SuperClass* exec( const kvs::ObjectBase* object );
};

//} // end of namespace StochasticStreamline
} // end of namespace local
