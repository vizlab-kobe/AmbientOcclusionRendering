#include "Input.h"
#include <kvs/DivergingColorMap>


namespace local
{

Input::Input( int argc, char** argv ):
    ssao( false ),
    lod( false ),
    ee( false ),
    scale( 100.0f ),
    repeats( 50 ),
    radius( 0.5f ),
    points( 256 ),
    min_coord( 0, 0, 0 ),
    max_coord( 250, 250, 250 ),
    stride( 30, 30, 30 ),
    tfunc( kvs::DivergingColorMap::CoolWarm( 256 ) ),
    shader( 0 )
{
    m_commandline = kvs::CommandLine( argc, argv );
    m_commandline.addHelpOption();
    m_commandline.addOption( "ssao","Enable SSAO (default: false).", 0, false );
    m_commandline.addOption( "lod","Enable LoD (default: false).", 0, false );
    m_commandline.addOption( "s", "Scaling factor (default: 1.0).", 1, false );
    m_commandline.addOption( "r", "Number of repeatitions (default: 50).", 1, false );
    m_commandline.addOption( "t", "Transfer function file (default: diverging colormap).", 1, false );
    m_commandline.addOption( "radius", "Sampling radius for SSAO (default: 0.5).", 1, false );
    m_commandline.addOption( "points", "Sampling points for SSAO (default: 256).", 1, false );
    m_commandline.addOption( "min_coord", "Min. object coordinates (default: 0 0 0).", 3, false );
    m_commandline.addOption( "max_coord", "Max. object coordinates (default: 250 250 250).", 3, false );
    m_commandline.addOption( "stride", "Grid stride for generating seed points (default: 30 30 30).", 3, false );
    m_commandline.addValue( "Input file (*.kvsml).", true );
}

bool Input::parse()
{
    if ( !m_commandline.parse() ) { return false; }

    ssao = m_commandline.hasOption( "ssao" );
    lod = m_commandline.hasOption( "lod" );
    if ( m_commandline.hasOption("s") ) scale = m_commandline.optionValue<float>("s");
    if ( m_commandline.hasOption("r") ) repeats = m_commandline.optionValue<size_t>("r");
    if ( m_commandline.hasOption("t") ) tfunc = kvs::TransferFunction( m_commandline.optionValue<std::string>("t") );
    if ( m_commandline.hasOption("radius") ) radius = m_commandline.optionValue<float>("radius");
    if ( m_commandline.hasOption("points") ) points = m_commandline.optionValue<int>("points");
    if ( m_commandline.hasOption("min_coord") )
    {
        min_coord[0] = m_commandline.optionValue<int>("min_coord", 0);
        min_coord[1] = m_commandline.optionValue<int>("min_coord", 1);
        min_coord[2] = m_commandline.optionValue<int>("min_coord", 2);
    }
    if ( m_commandline.hasOption("max_coord") )
    {
        max_coord[0] = m_commandline.optionValue<int>("max_coord", 0);
        max_coord[1] = m_commandline.optionValue<int>("max_coord", 1);
        max_coord[2] = m_commandline.optionValue<int>("max_coord", 2);
    }
    if ( m_commandline.hasOption("stride") )
    {
        stride[0] = m_commandline.optionValue<int>("stride", 0);
        stride[1] = m_commandline.optionValue<int>("stride", 1);
        stride[2] = m_commandline.optionValue<int>("stride", 2);
    }
    if ( m_commandline.hasValues() ) filename = m_commandline.value<std::string>();

    return true;
}

void Input::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Input file: " << filename << std::endl;
    os << indent << "SSAO: " << std::boolalpha << ssao << std::endl;
    os << indent << "LoD: " << std::boolalpha << lod << std::endl;
    os << indent << "Scale: " << scale << std::endl;
    os << indent << "Repeats: " << repeats << std::endl;
    os << indent << "Min. coord: " << min_coord << std::endl;
    os << indent << "Max. coord: " << max_coord << std::endl;
    os << indent << "Stride: " << stride << std::endl;
    os << indent << "Sampling radius: " << radius << std::endl;
    os << indent << "Sampling points: " << points << std::endl;
}

} // end of namespace local
