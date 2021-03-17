#pragma once
#include <string>
#include <kvs/Indent>
#include <kvs/Vector3>
#include <kvs/TransferFunction>
#include <kvs/CommandLine>


namespace local
{

struct Input
{
    bool ao = false; ///< flag for SSAO
    bool lod = false; ///< flag for LoD rendering
    size_t repeats = 20; ///< number of repetitions
    float radius = 0.5f; ///< sampling radius for SSAO
    int points = 256; ///< sampling points for SSAO
    float edge = 1.0f; ///< edge factor
    kvs::TransferFunction tfunc = kvs::ColorMap::BrewerSpectral( 256 ); ///< transfer function
    std::string filename = ""; ///< input filename

    Input() = default;

    bool parse( int argc, char** argv )
    {
        kvs::CommandLine cl( argc, argv );
        cl.addHelpOption();
        cl.addOption( "ao","Enable SSAO (default: false).", 0, false );
        cl.addOption( "lod","Enable LoD (default: false).", 0, false );
        cl.addOption( "r", "Number of repeatitions (default: 50).", 1, false );
        cl.addOption( "t", "Transfer function file (default: diverging colormap).", 1, false );
        cl.addOption( "radius", "Sampling radius for SSAO (default: 0.5).", 1, false );
        cl.addOption( "points", "Sampling points for SSAO (default: 256).", 1, false );
        cl.addOption( "edge", "Edge factor for SSAO (default: 1.0).", 1, false );
        cl.addValue( "Input file (*.kvsml).", true );
        if ( !cl.parse() ) { return false; }

        ao = cl.hasOption( "ao" );
        lod = cl.hasOption( "lod" );
        if ( cl.hasOption("r") ) repeats = cl.optionValue<size_t>("r");
        if ( cl.hasOption("t") ) tfunc = kvs::TransferFunction( cl.optionValue<std::string>("t") );
        if ( cl.hasOption("radius") ) radius = cl.optionValue<float>("radius");
        if ( cl.hasOption("points") ) points = cl.optionValue<int>("points");
        if ( cl.hasOption("edge") ) edge = cl.optionValue<float>("edge");
        if ( cl.hasValues() ) filename = cl.value<std::string>();

        return true;
    }
};

} // end of namespace local
