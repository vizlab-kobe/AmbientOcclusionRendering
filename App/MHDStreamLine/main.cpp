#include "Program.h"
#include <kvs/ShaderSource>


int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath( "../../Lib" );
    kvs::ShaderSource::AddSearchPath( "../../../StochasticStreamline/Lib" );

    local::Program program;
    return program.start( argc, argv );
}
