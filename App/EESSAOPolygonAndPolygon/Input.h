#pragma once
#include <kvs/Vector3>
#include <kvs/TransferFunction>
#include <kvs/CommandLine>
#include <string>


namespace local
{

struct Input
{
    std::string title = ""; ///< application title
    bool ao = false; ///< flag for SSAO
    bool lod = false; ///< flag for LoD rendering
    size_t repeats = 20; ///< number of repetitions
    float radius = 0.5f; ///< sampling radius for SSAO
    int points = 256; ///< sampling points for SSAO
    float edge = 1.0f; ///< edge factor
    float opacities[2] = { 0.5f, 0.5f }; ///< opacity of polygon object
    kvs::TransferFunction tfuncs[2] =
    {
        kvs::ColorMap::CoolWarm( 256 ), // for isosurface #0
        kvs::ColorMap::BrewerSpectral( 256 )  // for isosurface #1
    }; ///< transfer functions
    std::string labels[2] = { " ", " " }; ///< value label
    std::string filenames[2] = { "", "" }; ///< input filenames

    Input() = default;

    bool parse( int argc, char** argv )
    {
        kvs::CommandLine cl( argc, argv );
        cl.addHelpOption();
        cl.addOption( "ao","Enable SSAO (default: false).", 0, false );
        cl.addOption( "lod","Enable LoD (default: false).", 0, false );
        cl.addOption( "r", "Number of repeatitions (default: 50).", 1, false );
        cl.addOption( "radius", "Sampling radius for SSAO (default: 0.5).", 1, false );
        cl.addOption( "points", "Sampling points for SSAO (default: 256).", 1, false );
        cl.addOption( "edge", "Edge factor for SSAO (default: 1.0).", 1, false );
        cl.addOption( "title", "Application title (default: "").", 1, false );
        cl.addOption( "tfunc0", "Transfer function file for volume #0 (default: brewer spectral).", 1, false );
        cl.addOption( "tfunc1", "Transfer function file for volume #1 (default: brewer spectral).", 1, false );
        cl.addOption( "label0", "Value label for volume #0 (default: "").", 1, false );
        cl.addOption( "label1", "Value label for volume #1 (default: "").", 1, false );
        cl.addOption( "file0", "Input file for volume #0 (*.kvsml).", 1, true );
        cl.addOption( "file1", "Input file for volume #1 (*.kvsml).", 1, true );
        if ( !cl.parse() ) { return false; }

        ao = cl.hasOption( "ao" );
        lod = cl.hasOption( "lod" );
        if ( cl.hasOption("r") ) repeats = cl.optionValue<size_t>("r");
        if ( cl.hasOption("radius") ) radius = cl.optionValue<float>("radius");
        if ( cl.hasOption("points") ) points = cl.optionValue<int>("points");
        if ( cl.hasOption("edge") ) edge = cl.optionValue<float>("edge");
        if ( cl.hasOption("title") ) title = cl.optionValue<std::string>("title");
        if ( cl.hasOption("tfunc0") ) tfuncs[0] = kvs::TransferFunction( cl.optionValue<std::string>("tfunc0") );
        if ( cl.hasOption("tfunc1") ) tfuncs[1] = kvs::TransferFunction( cl.optionValue<std::string>("tfunc1") );
        if ( cl.hasOption("label0") ) labels[0] = cl.optionValue<std::string>("label0");
        if ( cl.hasOption("label1") ) labels[1] = cl.optionValue<std::string>("label1");
        if ( cl.hasOption("file0") ) filenames[0] = cl.optionValue<std::string>("file0");
        if ( cl.hasOption("file1") ) filenames[1] = cl.optionValue<std::string>("file1");

        return true;
    }
};

} // end of namespace local
