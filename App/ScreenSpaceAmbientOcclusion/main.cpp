/*****************************************************************************/
/**
 *  @file   main.cpp
 *  @author Naohisa Sakamoto
 *  @brief  Simple SSAO implementation using KVS.
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright (c) Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id$
 */
/*****************************************************************************/
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/PolygonImporter>
#include "PolygonToPolygon.h"
#include "SSAORenderer.h"
#include "SSAOPolygonRenderer.h"

/*
kvs::glew::SSAORenderer* ssao_renderer = NULL;
class KeyPressEvent : public kvs::KeyPressEventListener
{
    void update( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        //case kvs::Key::o:
            //screen()->controlTarget() = kvs::ScreenBase::TargetObject; break;
        //case kvs::Key::l:
            //screen()->controlTarget() = kvs::ScreenBase::TargetLight; break;
        case kvs::Key::d:
            ssao_renderer->enableDebugDraw(); break;
        case kvs::Key::D:
            ssao_renderer->disableDebugDraw(); break;
        default: break;
        }
    }
};
*/

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        kvsMessageError( "Please input Polygon file path." );
        return(0);
    }

    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setTitle( "Screen Space Ambient Occlusion" );

//    KeyPressEvent key_press_event;
//    screen.addEvent( &key_press_event );

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
//    typedef kvs::glew::SSAORenderer Renderer;
    Renderer* renderer = new Renderer();

    screen.registerObject( polygon, renderer );

    return( app.run() );
}
