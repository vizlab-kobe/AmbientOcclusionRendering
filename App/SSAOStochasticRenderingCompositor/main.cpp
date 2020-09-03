#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/TransferFunctionEditor>
#include <kvs/Slider>
#include <kvs/CheckBox>
#include <kvs/PolygonObject>
#include <kvs/ExternalFaces>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/UnstructuredVolumeImporter>
#include "SSAOStochasticPolygonRenderer.h"
#include "SSAOStochasticTetrahedraRenderer.h"
#include "SSAOStochasticRenderingCompositor.h"
#include <iostream>


/*===========================================================================*/
/**
 *  @brief  Main function.
 *  @param  argc [i] argument count
 *  @param  argv [i] argument values
 */
/*===========================================================================*/
int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::ShaderSource::AddSearchPath("../../../StochasticStreamline/Lib");

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle("SSAOStochasticRenderingCompositor Tetrahedra and Polygon");
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.show();

    // Import volume object.
    kvs::UnstructuredVolumeObject* volume_object = new kvs::UnstructuredVolumeImporter( argv[1] );
    volume_object->print( std::cout );

    // Declare SSAOStochasticTetrahedraRenderer.
    AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer* volume_renderer = new AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer();
    volume_renderer->setName("Renderer");

    // Generate polygon object from volume object.
    kvs::PolygonObject* polygon_object = new kvs::ExternalFaces( volume_object );
    polygon_object->setName( "Polygon" );
    polygon_object->setColor( kvs::RGBColor::White() );
    polygon_object->setOpacity( 128 );
    polygon_object->print( std::cout << std::endl );

    // Declare SSAOStochasticPolygonRenderer.
    AmbientOcclusionRendering::SSAOStochasticPolygonRenderer* polygon_renderer = new AmbientOcclusionRendering::SSAOStochasticPolygonRenderer();
    polygon_renderer->setPolygonOffset( 0.001f );

    // Register objects and renderers
    screen.registerObject( volume_object, volume_renderer );
    screen.registerObject( polygon_object, polygon_renderer );

    // Declare SSAOStochasticRenderingCompositor
    AmbientOcclusionRendering::SSAOStochasticRenderingCompositor compositor( screen.scene() );
    compositor.setRepetitionLevel( 50 );
    compositor.enableLODControl();
    screen.setEvent( &compositor );

    // Widgets.
    kvs::TransferFunctionEditor editor( &screen );
    editor.setPosition( screen.x() + screen.width(), screen.y() );
    editor.setVolumeObject( volume_object );
    editor.apply(
        [&]( kvs::TransferFunction tfunc ) {
            volume_renderer->setTransferFunction( tfunc );
            screen.redraw();
        } );
    editor.show();

    kvs::CheckBox checkbox( &screen );
    checkbox.setCaption( "LOD" );
    checkbox.setMargin( 10 );
    checkbox.setState( true );
    checkbox.anchorToTopLeft();
    checkbox.stateChanged(
        [&]() {
            compositor.setEnabledLODControl( checkbox.state() );
            screen.redraw();
        } );
    checkbox.show();

    kvs::Slider opacity( &screen );
    opacity.setCaption( "Opacity" );
    opacity.setWidth( 150 );
    opacity.setMargin( 10 );
    opacity.setValue( 0.5 );
    opacity.setRange( 0, 1 );
    opacity.anchorToBottom( &checkbox );
    opacity.valueChanged(
        [&]() {
            auto* scene = screen.scene();
            auto* object1 = kvs::PolygonObject::DownCast( scene->object( "Polygon" ) );
            auto* object2 = new kvs::PolygonObject();
            object2->shallowCopy( *object1 );
            object2->setName( "Polygon" );
            object2->setOpacity( int( opacity.value() * 255 + 0.5 ) );
            scene->replaceObject( "Polygon", object2 );
        } );
    opacity.show();

    kvs::Slider repetition( &screen );
    repetition.setCaption( "Repetition" );
    repetition.setWidth( 150 );
    repetition.setMargin( 10 );
    repetition.setValue( 50 );
    repetition.setRange( 1, 100 );
    repetition.anchorToBottom( &opacity );
    repetition.valueChanged(
        [&]() {
            compositor.setRepetitionLevel( int( repetition.value() + 0.5 ) );
            screen.redraw();
        } );
    repetition.show();

    return app.run();
}

/*===========================================================================*/
/**
 *  @brief  Transfer function editor.
 */
/*===========================================================================*/
/*class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
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
    };*/

/*===========================================================================*/
/**
 *  @brief  LOD check box.
 */
/*===========================================================================*/
/*class LODCheckBox : public kvs::glut::CheckBox
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
    };*/

/*===========================================================================*/
/**
 *  @brief  Opacity slider.
 */
/*===========================================================================*/
/*class OpacitySlider : public kvs::glut::Slider
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
    };*/

/*===========================================================================*/
/**
 *  @brief  Repetition slider.
 */
/*===========================================================================*/
/*class RepetitionSlider : public kvs::glut::Slider
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
    };*/
