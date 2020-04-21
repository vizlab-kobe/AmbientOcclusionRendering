#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/glut/TransferFunctionEditor>
#include <kvs/glut/Slider>
#include <kvs/glut/CheckBox>
#include <kvs/PolygonObject>
#include <kvs/ExternalFaces>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/UnstructuredVolumeImporter>
#include "SSAOStochasticPolygonRenderer.h"
#include "SSAOStochasticTetrahedraRenderer.h"
#include "SSAOStochasticRenderingCompositor.h"
#include <kvs/Scene>
#include <kvs/ObjectManager>
#include <kvs/RendererManager>
#include <iostream>


/*===========================================================================*/
/**
 *  @brief  Transfer function editor.
 */
/*===========================================================================*/
class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{
public:

    TransferFunctionEditor( kvs::glut::Screen* screen ):
        kvs::glut::TransferFunctionEditor( screen ){}

    void apply()
    {
        typedef AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer Renderer;
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        Renderer* renderer = static_cast<Renderer*>( scene->rendererManager()->renderer( "Renderer" ) );
        renderer->setTransferFunction( transferFunction() );
        screen()->redraw();
    }
};

/*===========================================================================*/
/**
 *  @brief  LOD check box.
 */
/*===========================================================================*/
class LODCheckBox : public kvs::glut::CheckBox
{
    AmbientOcclusionRendering::SSAOStochasticRenderingCompositor* m_compositor;

public:

    LODCheckBox( kvs::glut::Screen* screen, AmbientOcclusionRendering::SSAOStochasticRenderingCompositor* compositor ):
        kvs::glut::CheckBox( screen ),
        m_compositor( compositor )
    {
        setMargin( 10 );
        setCaption( "Level-of-Detail" );
    }

    void stateChanged()
    {
        m_compositor->setEnabledLODControl( state() );
        screen()->redraw();
    }
};

/*===========================================================================*/
/**
 *  @brief  Opacity slider.
 */
/*===========================================================================*/
class OpacitySlider : public kvs::glut::Slider
{
public:

    OpacitySlider( kvs::glut::Screen* screen ):
        kvs::glut::Slider( screen )
    {
        setWidth( 150 );
        setMargin( 10 );
        setCaption( "Opacity" );
    }

    void valueChanged()
    {
        typedef kvs::PolygonObject Object;
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        Object* object1 = Object::DownCast( scene->objectManager()->object( "Polygon" ) );
        Object* object2 = new Object();
        object2->shallowCopy( *object1 );
        object2->setName( "Polygon" );
        object2->setOpacity( int( value() * 255 + 0.5 ) );
        scene->objectManager()->change( "Polygon", object2 );
    }
};

/*===========================================================================*/
/**
 *  @brief  Repetition slider.
 */
/*===========================================================================*/
class RepetitionSlider : public kvs::glut::Slider
{
    AmbientOcclusionRendering::SSAOStochasticRenderingCompositor* m_compositor;

public:

    RepetitionSlider( kvs::glut::Screen* screen, AmbientOcclusionRendering::SSAOStochasticRenderingCompositor* compositor ):
        kvs::glut::Slider( screen ),
        m_compositor( compositor )
    {
        setWidth( 150 );
        setMargin( 10 );
        setCaption( "Repetition" );
    }

    void valueChanged()
    {
        m_compositor->setRepetitionLevel( int( value() + 0.5 ) );
        screen()->redraw();
    }
};

/*===========================================================================*/
/**
 *  @brief  Main function.
 *  @param  argc [i] argument count
 *  @param  argv [i] argument values
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    kvs::glut::Application app( argc, argv );
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::ShaderSource::AddSearchPath("../../../StochasticStreamline/Lib");

    kvs::UnstructuredVolumeObject* volume_object = new kvs::UnstructuredVolumeImporter( argv[1] );
    volume_object->print( std::cout );

    AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer* volume_renderer = new AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer();
    volume_renderer->setName("SSAOStochasticRenderingCompositor");

    kvs::PolygonObject* polygon_object = new kvs::ExternalFaces( volume_object );
    polygon_object->setName( "Polygon" );
    polygon_object->setColor( kvs::RGBColor::White() );
    polygon_object->setOpacity( 128 );
    polygon_object->print( std::cout << std::endl );

    AmbientOcclusionRendering::SSAOStochasticPolygonRenderer* polygon_renderer = new AmbientOcclusionRendering::SSAOStochasticPolygonRenderer();
    polygon_renderer->setPolygonOffset( 0.001f );

    kvs::glut::Screen screen( &app );
    screen.setTitle("Example program for kvs::SSAOStochasticRenderingCompositor");
    screen.registerObject( polygon_object, polygon_renderer );
    screen.registerObject( volume_object, volume_renderer );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.show();

    AmbientOcclusionRendering::SSAOStochasticRenderingCompositor compositor( screen.scene() );
    compositor.setRepetitionLevel( 50 );
    compositor.enableLODControl();
    screen.setEvent( &compositor );

    TransferFunctionEditor editor( &screen );
    editor.setVolumeObject( volume_object );
    editor.show();

    LODCheckBox checkbox( &screen, &compositor );
    checkbox.setPosition( 0, 0 );
    checkbox.setState( true );
    checkbox.show();

    OpacitySlider opacity( &screen );
    opacity.setPosition( checkbox.x(), checkbox.y() + checkbox.height() );
    opacity.setValue( 0.5 );
    opacity.setRange( 0, 1 );
    opacity.show();

    RepetitionSlider repetition( &screen, &compositor );
    repetition.setPosition( opacity.x(), opacity.y() + opacity.height() );
    repetition.setValue( 50 );
    repetition.setRange( 1, 100 );
    repetition.show();

    return app.run();
}