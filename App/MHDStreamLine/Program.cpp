#include "Program.h"
#include "Input.h"
#include "Vis.h"
#include "Widget.h"
#include "Event.h"
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>


namespace local
{

int Program::exec( int argc, char** argv )
{
    // Input variables.
    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }

    input.print( std::cout << "Input Variables" << std::endl, kvs::Indent( 4 ) );

    // Application and screen.
    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setTitle( "Stochastic Tubeline Rendering with SSAO" );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.show();

    // Visualization pipeline.
    local::Vis vis( input );
    kvs::StructuredVolumeObject* volume = vis.import();
    kvs::LineObject* object = vis.streamline( volume );
    kvs::RendererBase* renderer = vis.renderer();
    screen.registerObject( object, renderer );

    volume->print( std::cout << "Imported Volume Object" << std::endl, kvs::Indent( 4 ) );
    object->print( std::cout << "Generated Streamlines" << std::endl, kvs::Indent( 4 ) );

    // Widgets.
    local::Widget widget( screen, input, vis );
    widget.show();

    // Events.
    local::Event local_event( widget );
    kvs::ScreenCaptureEvent capture_event;
    kvs::TargetChangeEvent target_change_event;
    screen.addEvent( &local_event );
    screen.addEvent( &capture_event );
    screen.addEvent( &target_change_event );

    return app.run();
}

} // end of namespace local
