#pragma once
#include <string>
#include <kvs/CommandLine>
#include <kvs/Indent>
#include <kvs/Vector3>
#include <kvs/TransferFunction>


namespace local
{

class Input
{
private:
    kvs::CommandLine m_commandline;

public:
    std::string filename; ///< input filename
    bool ssao; ///< flag for SSAO
    bool lod; ///< flag for LoD rendering
    float scale; ///< scaling factor
    size_t repeats; ///< number of repetitions
    float radius; ///< sampling radius for SSAO
    int points; ///< sampling points for SSAO
    kvs::Vec3i min_coord; ///< min. coords
    kvs::Vec3i max_coord; ///< max. coords
    kvs::Vec3i stride; ///< grid stride for generating seed points
    kvs::TransferFunction tfunc; ///< transfer function

public:
    Input( int argc, char** argv );
    bool parse();
    void print( std::ostream& os, const kvs::Indent& indent ) const;
};

} // end of namespace local
