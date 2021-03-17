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
    float scale = 100.0f; ///< scaling factor
    size_t repeats = 20; ///< number of repetitions
    float radius = 0.5f; ///< sampling radius for SSAO
    int points = 256; ///< sampling points for SSAO
    kvs::Vec3i min_coord = kvs::Vec3i( 0, 0, 0 ); ///< min. coords
    kvs::Vec3i max_coord = kvs::Vec3i( 250, 250, 250 ); ///< max. coords
    kvs::Vec3i stride = kvs::Vec3i( 30, 30, 30 ); ///< grid stride for generating seed points
    kvs::TransferFunction tfunc = kvs::ColorMap::BrewerSpectral( 256 ); ///< transfer function
    float edge = 1.0f; ///< edge factor
    std::string filename = ""; ///< input filename

    Input() = default;

    bool parse( int argc, char** argv )
    {
        kvs::CommandLine cl( argc, argv );
        cl.addHelpOption();
        cl.addOption( "ao","Enable SSAO (default: false).", 0, false );
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

        ao = cl.hasOption( "ao" );
        lod = cl.hasOption( "lod" );
        if ( cl.hasOption("s") ) scale = cl.optionValue<float>("s");
        if ( cl.hasOption("r") ) repeats = cl.optionValue<size_t>("r");
        if ( cl.hasOption("t") ) tfunc = kvs::TransferFunction( cl.optionValue<std::string>("t") );
        if ( cl.hasOption("radius") ) radius = cl.optionValue<float>("radius");
        if ( cl.hasOption("points") ) points = cl.optionValue<int>("points");
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
        if ( cl.hasValues() ) filename = cl.value<std::string>();

        return true;
    }
/*
private:
    std::string m_filename; ///< input filename
    bool m_ssao_enabled; ///< flag for SSAO
    bool m_lod_enabled; ///< flag for LoD rendering
    float m_scale; ///< scaling factor
    size_t m_repeats; ///< number of repetitions
    float m_radius; ///< sampling radius for SSAO
    int m_points; ///< sampling points for SSAO
    kvs::Vec3i m_min_coord; ///< min. coords
    kvs::Vec3i m_max_coord; ///< max. coords
    kvs::Vec3i m_stride; ///< grid stride for generating seed points
    kvs::TransferFunction m_tfunc; ///< transfer function
    float m_edge_factor;

public:
    Input();

    const std::string& filename() const { return m_filename; }
    bool isSSAOEnabled() const { return m_ssao_enabled; }
    bool isLODEnabled() const { return m_lod_enabled; }
    float scale() const { return m_scale; }
    size_t repeats() const { return m_repeats; }
    float radius() const { return m_radius; }
    int points() const { return m_points; }
    float edge() const { return m_edge_factor; }
    const kvs::Vec3i& minCoord() const { return m_min_coord; }
    const kvs::Vec3i& maxCoord() const { return m_max_coord; }
    const kvs::Vec3i& stride() const { return m_stride; }
    const kvs::TransferFunction& transferFunction() const { return m_tfunc; }

    void setFilename( const std::string& filename ) { m_filename = filename; }
    void setSSAOEnabled( const bool enable = true ) { m_ssao_enabled = enable; }
    void setLODEnabled( const bool enable = true ) { m_lod_enabled =enable; }
    void setRepeats( const size_t repeats ) { m_repeats = repeats; }
    void setRadius( const float radius ) { m_radius = radius; }
    void setPoints( const int points ) { m_points = points; }
    void setMinCoord( const kvs::Vec3i& coord ) { m_min_coord = coord; }
    void setMaxCoord( const kvs::Vec3i& coord ) { m_max_coord = coord; }
    void setStride( const kvs::Vec3i& stride ) { m_stride = stride; }
    void setTransferFunction( const kvs::TransferFunction& tfunc ) { m_tfunc = tfunc; }
    void setEdgeFactor( const float edge_factor ) { m_edge_factor = edge_factor; }

    bool parse( int argc, char** argv );
    void print( std::ostream& os, const kvs::Indent& indent ) const;
*/
};

} // end of namespace local
