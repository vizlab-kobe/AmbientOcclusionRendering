#include "Program.h"
#include "Input.h"
#include "Model.h"
#include "View.h"
#include "Controller.h"
#include <kvs/Application>


namespace local
{

int Program::exec( int argc, char** argv )
{
    kvs::Application app( argc, argv );

    local::Input input;
    if ( !input.parse( argc, argv ) ) { return 1; }

    local::Model model( input );
    local::View view( &app, &model );
    local::Controller controller( model, view );

    return app.run();
}

} // end of namespace local
