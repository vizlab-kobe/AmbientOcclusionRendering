#include <kvs/Application>
#include <kvs/Program>
#include "Input.h"
#include "Model.h"
#include "View.h"
#include "Controller.h"

/*===========================================================================*/
/**
 *  @brief  Main function
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath( "../../Lib" );
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
