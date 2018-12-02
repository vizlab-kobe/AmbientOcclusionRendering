/*****************************************************************************/
/**
 *  @file   main.cpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/ShaderSource>
#include <kvs/PolygonImporter>
#include <AmbientOcclusionRendering/Lib/PolygonToPolygon.h>
#include <AmbientOcclusionRendering/Lib/SSAOPolygonRenderer.h>


int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");

    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setTitle( "Screen Space Ambient Occlusion" );
    screen.show();

    kvs::PolygonObject* polygon = new kvs::PolygonImporter( argv[1] );
    const size_t nvertices = polygon->numberOfVertices();
    const size_t npolygons = polygon->numberOfConnections();
    if ( npolygons > 0 && nvertices != 3 * npolygons )
    {
        kvs::PolygonObject* temp = new AmbientOcclusionRendering::PolygonToPolygon( polygon );
        delete polygon;
        polygon = temp;
    }

    typedef AmbientOcclusionRendering::SSAOPolygonRenderer Renderer;
    Renderer* renderer = new Renderer();

    screen.registerObject( polygon, renderer );

    return app.run();
}
