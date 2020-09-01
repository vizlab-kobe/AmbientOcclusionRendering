#include "Program.h"
#include "Input.h"
#include "Model.h"
#include "View.h"
#include "Controller.h"
#include "Event.h"
#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>


namespace local
{

int Program::exec( int argc, char** argv )
{
    kvs::Application app( argc, argv );

    // Input variables.
    local::Input input;
    if ( !input.parse( argc, argv ) ) { return 1; }

    input.print( std::cout << "Input Variables" << std::endl, kvs::Indent( 4 ) );

    // Application and screen.
//    kvs::Application app( argc, argv );
//    kvs::Screen screen( &app );
//    screen.setTitle( "Stochastic Tubeline Rendering with SSAO" );
//    screen.setBackgroundColor( kvs::RGBColor::White() );
//    screen.show();

    // Visualization pipeline.
    local::Model model( input );
//    auto* volume = model.import();
//    auto* object = model.streamline( volume );
//    auto* renderer = model.renderer();
//    screen.registerObject( object, renderer );
    local::View view( &app, &model );

//    volume->print( std::cout << "Imported Volume Object" << std::endl, kvs::Indent( 4 ) );
//    object->print( std::cout << "Generated Streamlines" << std::endl, kvs::Indent( 4 ) );

    // Widgets.
//    local::Controller widget( screen, model );
    local::Controller controller( model, view );
    controller.show();

    // Events.
    local::Event local_event( controller );
    kvs::ScreenCaptureEvent capture_event;
    kvs::TargetChangeEvent target_change_event;
    view.screen().addEvent( &local_event );
    view.screen().addEvent( &capture_event );
    view.screen().addEvent( &target_change_event );

    return app.run();
}

} // end of namespace local
