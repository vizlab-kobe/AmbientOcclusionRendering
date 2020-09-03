#pragma once
#include <kvs/OpenGL>
#include <kvs/ValueArray>
#include <kvs/Xorshift128>
#include <cmath>


namespace local
{

kvs::ValueArray<GLfloat> SSAOPointSampling( const float radius, const size_t nsamples )
{
    kvs::Xorshift128 rand;
    kvs::ValueArray<GLfloat> sampling_points( 3 * nsamples );
    for ( size_t i = 0; i < nsamples ; i++ )
    {
        const float pi = 3.1415926f;

/*
        const float r = rand() * radius * 0.5f + 0.5f;
        const float u = rand() * 0.5f + 0.5f;
        const float v = rand();
        const float R = r * r;
        const float x = R * std::sqrt( 1.0 - v * v ) * std::cos( 2.0 * pi * u );
        const float y = R * std::sqrt( 1.0 - v * v ) * std::sin( 2.0 * pi * u );
        const float z = R * v;
*/
        const float r = radius * rand();
        const float t = 2.0f * pi * rand();
        const float cp = 2.0f * rand() - 1.0f;
        const float sp = std::sqrt( 1.0f - cp * cp );
        const float ct = std::cos( t );
        const float st = std::sin( t );
        const float x = r * sp * ct;
        const float y = r * sp * st;
        const float z = r * cp;

        sampling_points[ 3 * i + 0 ] = x;
        sampling_points[ 3 * i + 1 ] = y;
        sampling_points[ 3 * i + 2 ] = z;
    }

    return sampling_points;
}

} // end of namespace AmbientOcclusionRendering
