#include "View.h"
#include <kvs/RGBColor>
#include <kvs/Indent>


namespace local
{

View::View( kvs::Application* app, local::Model* model ):
    m_model( model ),
    m_screen( app )
{
    this->setup();
    this->show();
}

void View::show()
{
    m_screen.show();
}

void View::redraw()
{
    m_screen.redraw();
}

void View::setup()
{
    m_screen.setTitle( "MHD Streamline" );
    m_screen.setBackgroundColor( kvs::RGBColor::White() );

    auto* volume = m_model->import();
    auto* object = m_model->streamline( volume );
    auto* renderer = m_model->renderer();
    //m_screen.setSize( 1024, 1024 );
    m_screen.registerObject( object, renderer );

    const kvs::Indent indent(4);
    volume->print( std::cout << "Imported Volume Object" << std::endl, indent );
    object->print( std::cout << "Generated Streamlines" << std::endl, indent );
}

} // end of namespace local
