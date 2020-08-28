#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <kvs/ValueArray>
#include <kvs/Endian>


namespace local
{

inline kvs::ValueArray<float> ReadBinary( const std::string& filename, const size_t offset, const bool swap )
  {
    std::ifstream ifs( filename.c_str(), std::ifstream::binary );

    ifs.seekg( 0, ifs.end );
    const size_t file_size = ifs.tellg();
    const size_t data_size = file_size - offset * 2;

    ifs.seekg( offset, ifs.beg );
    kvs::ValueArray<float> values( data_size / sizeof( float ) );
    ifs.read( reinterpret_cast<char*>( values.data() ), values.byteSize() );
    if ( swap ) { kvs::Endian::Swap( values.data(), values.size() ); }

    return values;
  }

}
