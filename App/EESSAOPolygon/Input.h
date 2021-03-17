#pragma once
#include <kvs/CommandLine>
#include <string>


namespace local
{

struct Input
{
    bool ao = false; ///< AO flag
    bool lod = false; ///< LoD flag
    size_t repeats = 20; ///< number of repetitions for stochasti rendering
    float radius = 0.5f; ///< radius of point sampling region for SSAO
    int points = 256; ///< number of points used for SSAO
    float opacity = 0.1f; ///< opacity of polygon object
    float edge = 1.0f; ///< edge factor
    std::string filename = ""; ///< input filename

    Input() = default;

    bool parse( int argc, char** argv )
    {
        kvs::CommandLine cl( argc, argv );
        cl.addHelpOption();
        cl.addOption( "ao","Enable SSAO (default: false).", 0, false );
        cl.addOption( "lod","Enable LoD (default: false).", 0, false );
        cl.addOption( "r", "Number of repeatitions (default: 20).", 1, false );
        cl.addOption( "radius", "Sampling radius for SSAO (default: 0.5).", 1, false );
        cl.addOption( "points", "Sampling points for SSAO (default: 256).", 1, false );
        cl.addOption( "edge", "Edge factor for SSAO (default: 1.0).", 1, false );
        cl.addValue( "Input file (*.kvsml).", true );
        if ( !cl.parse() ) { return false; }

        ao = cl.hasOption( "ao" );
        lod = cl.hasOption( "lod" );
        if ( cl.hasOption("r") ) repeats = cl.optionValue<size_t>("r");
        if ( cl.hasOption("radius") ) radius = cl.optionValue<float>("radius");
        if ( cl.hasOption("points") ) points = cl.optionValue<int>("points");
        if ( cl.hasOption("edge") ) edge = cl.optionValue<float>("edge");
        if ( cl.hasValues() ) filename = cl.value<std::string>();

        return true;
    }
};

} // end of namespace local
