#include "Input.h"
#include <kvs/DivergingColorMap>
#include <kvs/CommandLine>


namespace local
{

Input::Input():
    m_ssao_enabled( false ),
    m_lod_enabled( false ),
    m_scale( 1000.0f ),
    m_repeats( 50 ),
    m_radius( 0.5f ),
    m_points( 256 ),
    m_min_coord( 0, 0, 0 ),
    m_max_coord( 250, 250, 250 ),
    m_stride( 30, 30, 30 ),
    m_tfunc( kvs::DivergingColorMap::CoolWarm( 256 ) ),
    m_edge_factor( 1.0f )
{
}

bool Input::parse( int argc, char** argv )
{
    kvs::CommandLine cl( argc, argv );
    cl.addHelpOption();
    cl.addOption( "ssao","Enable SSAO (default: false).", 0, false );
    cl.addOption( "lod","Enable LoD (default: false).", 0, false );
    cl.addOption( "s", "Scaling factor (default: 1.0).", 1, false );
    cl.addOption( "r", "Number of repeatitions (default: 50).", 1, false );
    cl.addOption( "t", "Transfer function file (default: diverging colormap).", 1, false );
    cl.addOption( "radius", "Sampling radius for SSAO (default: 0.5).", 1, false );
    cl.addOption( "points", "Sampling points for SSAO (default: 256).", 1, false );
    cl.addOption( "min_coord", "Min. object coordinates (default: 0 0 0).", 3, false );
    cl.addOption( "max_coord", "Max. object coordinates (default: 250 250 250).", 3, false );
    cl.addOption( "stride", "Grid stride for generating seed points (default: 30 30 30).", 3, false );
    cl.addValue( "Input file (*.kvsml).", true );
    if ( !cl.parse() ) { return false; }

    m_ssao_enabled = cl.hasOption( "ssao" );
    m_lod_enabled = cl.hasOption( "lod" );
    if ( cl.hasOption("s") ) m_scale = cl.optionValue<float>("s");
    if ( cl.hasOption("r") ) m_repeats = cl.optionValue<size_t>("r");
    if ( cl.hasOption("t") ) m_tfunc = kvs::TransferFunction( cl.optionValue<std::string>("t") );
    if ( cl.hasOption("radius") ) m_radius = cl.optionValue<float>("radius");
    if ( cl.hasOption("points") ) m_points = cl.optionValue<int>("points");
    if ( cl.hasOption("min_coord") )
    {
        m_min_coord[0] = cl.optionValue<int>("min_coord", 0);
        m_min_coord[1] = cl.optionValue<int>("min_coord", 1);
        m_min_coord[2] = cl.optionValue<int>("min_coord", 2);
    }
    if ( cl.hasOption("max_coord") )
    {
        m_max_coord[0] = cl.optionValue<int>("max_coord", 0);
        m_max_coord[1] = cl.optionValue<int>("max_coord", 1);
        m_max_coord[2] = cl.optionValue<int>("max_coord", 2);
    }
    if ( cl.hasOption("stride") )
    {
        m_stride[0] = cl.optionValue<int>("stride", 0);
        m_stride[1] = cl.optionValue<int>("stride", 1);
        m_stride[2] = cl.optionValue<int>("stride", 2);
    }
    if ( cl.hasValues() ) m_filename = cl.value<std::string>();

    return true;
}

void Input::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Input file: " << m_filename << std::endl;
    os << indent << "SSAO: " << std::boolalpha << m_ssao_enabled << std::endl;
    os << indent << "LoD: " << std::boolalpha << m_lod_enabled << std::endl;
    os << indent << "Scale: " << m_scale << std::endl;
    os << indent << "Repeats: " << m_repeats << std::endl;
    os << indent << "Min. coord: " << m_min_coord << std::endl;
    os << indent << "Max. coord: " << m_max_coord << std::endl;
    os << indent << "Stride: " << m_stride << std::endl;
    os << indent << "Sampling radius: " << m_radius << std::endl;
    os << indent << "Sampling points: " << m_points << std::endl;
}

} // end of namespace local
