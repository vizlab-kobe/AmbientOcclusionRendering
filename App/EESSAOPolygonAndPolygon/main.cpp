#include <kvs/ShaderSource>
#include <kvs/Program>
#include <kvs/Application>
#include "Input.h"
#include "Model.h"
#include "View.h"
#include "Controller.h"


int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");

    return kvs::Program( [&] ()
    {
        // Application
        kvs::Application app( argc, argv );

        // Input parameters
        local::Input input;
        if ( !input.parse( argc, argv ) ) { return 1; }

        // MVC
        local::Model model( input );
        local::View view( app, model );
        local::Controller controller( model, view );

        return app.run();
    } ).run();
}
