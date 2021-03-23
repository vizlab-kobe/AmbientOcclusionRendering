#pragma once
#include <kvs/Vector3>
#include <kvs/TransferFunction>
#include <kvs/CommandLine>
#include <string>


namespace local
{

struct Input
{
    enum MappingMethod { Isosurface = 0, Streamline = 1 };

    bool ao = false; ///< flag for SSAO
    bool lod = false; ///< flag for LoD rendering
    float scale = 100.0f; ///< scaling factor
    size_t repeats = 20; ///< number of repetitions
    float radius = 0.5f; ///< sampling radius for SSAO
    int points = 256; ///< sampling points for SSAO
    float opacity = 0.5f; ///< opacity of polygon object
    float edge = 1.0f; ///< edge factor
    kvs::Vec3i min_coord = kvs::Vec3i( 0, 0, 0 ); ///< min. coords
    kvs::Vec3i max_coord = kvs::Vec3i( 250, 250, 250 ); ///< max. coords
    kvs::Vec3i stride = kvs::Vec3i( 30, 30, 30 ); ///< grid stride for generating seed points
    std::string title = ""; ///< application title
    kvs::TransferFunction tfuncs[2] =
    {
        kvs::ColorMap::CoolWarm( 256 ), // for isosurface
        kvs::ColorMap::BrewerSpectral( 256 )  // for streamline
    }; ///< transfer functions
    std::string labels[2] = { " ", " " }; ///< value label
    std::string filenames[2] = { "", "" }; ///< input filenames for isosurface and streamline

    Input() = default;

    bool parse( int argc, char** argv )
    {
        kvs::CommandLine cl( argc, argv );
        cl.addHelpOption();
        cl.addOption( "ao","Enable SSAO (default: false).", 0, false );
        cl.addOption( "lod","Enable LoD (default: false).", 0, false );
        cl.addOption( "s", "Scaling factor (default: 1.0).", 1, false );
        cl.addOption( "r", "Number of repeatitions (default: 50).", 1, false );
        cl.addOption( "radius", "Sampling radius for SSAO (default: 0.5).", 1, false );
        cl.addOption( "points", "Sampling points for SSAO (default: 256).", 1, false );
        cl.addOption( "edge", "Edge factor for SSAO (default: 1.0).", 1, false );
        cl.addOption( "min_coord", "Min. object coordinates (default: 0 0 0).", 3, false );
        cl.addOption( "max_coord", "Max. object coordinates (default: 250 250 250).", 3, false );
        cl.addOption( "stride", "Grid stride for generating seed points (default: 30 30 30).", 3, false );
        cl.addOption( "title", "Application title (default: "").", 1, false );
        cl.addOption( "isosurface_tfunc", "Transfer function file for isosurface (default: brewer spectral).", 1, false );
        cl.addOption( "streamline_tfunc", "Transfer function file for streamline (default: brewer spectral).", 1, false );
        cl.addOption( "isosurface_label", "Value label for isosurface (default: "").", 1, false );
        cl.addOption( "streamline_label", "Value label for streamline (default: "").", 1, false );
        cl.addOption( "isosurface_file", "Input file for isosurface (*.kvsml).", 1, true );
        cl.addOption( "streamline_file", "Input file for streamline (*.kvsml).", 1, true );
        if ( !cl.parse() ) { return false; }

        ao = cl.hasOption( "ao" );
        lod = cl.hasOption( "lod" );
        if ( cl.hasOption("s") ) scale = cl.optionValue<float>("s");
        if ( cl.hasOption("r") ) repeats = cl.optionValue<size_t>("r");
        if ( cl.hasOption("radius") ) radius = cl.optionValue<float>("radius");
        if ( cl.hasOption("points") ) points = cl.optionValue<int>("points");
        if ( cl.hasOption("edge") ) edge = cl.optionValue<float>("edge");
        if ( cl.hasOption("min_coord") )
        {
            min_coord[0] = cl.optionValue<int>("min_coord", 0);
            min_coord[1] = cl.optionValue<int>("min_coord", 1);
            min_coord[2] = cl.optionValue<int>("min_coord", 2);
        }
        if ( cl.hasOption("max_coord") )
        {
            max_coord[0] = cl.optionValue<int>("max_coord", 0);
            max_coord[1] = cl.optionValue<int>("max_coord", 1);
            max_coord[2] = cl.optionValue<int>("max_coord", 2);
        }
        if ( cl.hasOption("stride") )
        {
            stride[0] = cl.optionValue<int>("stride", 0);
            stride[1] = cl.optionValue<int>("stride", 1);
            stride[2] = cl.optionValue<int>("stride", 2);
        }
        if ( cl.hasOption("title") ) title = cl.optionValue<std::string>("title");
        if ( cl.hasOption("isosurface_tfunc") ) tfuncs[Isosurface] = kvs::TransferFunction( cl.optionValue<std::string>("isosurface_tfunc") );
        if ( cl.hasOption("streamline_tfunc") ) tfuncs[Streamline] = kvs::TransferFunction( cl.optionValue<std::string>("streamline_tfunc") );
        if ( cl.hasOption("isosurface_label") ) labels[Isosurface] = cl.optionValue<std::string>("isosurface_label");
        if ( cl.hasOption("streamline_label") ) labels[Streamline] = cl.optionValue<std::string>("streamline_label");
        if ( cl.hasOption("isosurface_file") ) filenames[Isosurface] = cl.optionValue<std::string>("isosurface_file");
        if ( cl.hasOption("streamline_file") ) filenames[Streamline] = cl.optionValue<std::string>("streamline_file");

        return true;
    }
};

} // end of namespace local
