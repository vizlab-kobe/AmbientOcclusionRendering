#pragma once
#include <string>
#include <kvs/Indent>
#include <kvs/Vector3>
#include <kvs/TransferFunction>


namespace local
{

class Input
{
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

public:
    Input();

    const std::string& filename() const { return m_filename; }
    bool isSSAOEnabled() const { return m_ssao_enabled; }
    bool isLODEnabled() const { return m_lod_enabled; }
    float scale() const { return m_scale; }
    size_t repeats() const { return m_repeats; }
    float radius() const { return m_radius; }
    int points() const { return m_points; }
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

    bool parse( int argc, char** argv );
    void print( std::ostream& os, const kvs::Indent& indent ) const;
};

} // end of namespace local
