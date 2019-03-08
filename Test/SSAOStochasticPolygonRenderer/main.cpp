#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/PolygonImporter>
#include <kvs/PolygonRenderer>
#include <kvs/LineObject>
#include <kvs/ShaderSource>
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
#include <AmbientOcclusionRendering/Lib/PolygonToPolygon.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticPolygonRenderer.h>
#include <kvs/StochasticPolygonRenderer>
typedef AmbientOcclusionRendering::SSAOStochasticPolygonRenderer SSAORenderer;
typedef kvs::StochasticPolygonRenderer NoSSAORenderer;


namespace
{
float Opacity = 0.5f;
float SamplingRadius = 0.5f;
size_t SamplingPoints = 256;
}

namespace
{

inline kvs::PolygonObject* ImportPolygonObject( const std::string filename )
{
    kvs::PolygonObject* polygon = new kvs::PolygonImporter( filename );
    const size_t nvertices = polygon->numberOfVertices();
    const size_t npolygons = polygon->numberOfConnections();
    if ( npolygons > 0 && nvertices != 3 * npolygons )
    {
        kvs::PolygonObject* temp = new AmbientOcclusionRendering::PolygonToPolygon( polygon );
        delete polygon;
        polygon = temp;
    }
    polygon->setName( "Object" );
    return polygon;
}

inline kvs::RendererBase* CreateRenderer( const bool ssao, const size_t repeats )
{
    if ( ssao )
    {
        SSAORenderer* renderer = new SSAORenderer();
        renderer->setName( "Renderer" );
        renderer->setRepetitionLevel( repeats );
        renderer->setEnabledLODControl( true );
        renderer->setSamplingSphereRadius( ::SamplingRadius );
        renderer->setNumberOfSamplingPoints( ::SamplingPoints );
        renderer->enableShading();
        return renderer;
    }
    else
    {
        NoSSAORenderer* renderer = new NoSSAORenderer();
        renderer->setName( "Renderer" );
        renderer->setRepetitionLevel( repeats );
        renderer->setEnabledLODControl( true );
        renderer->enableShading();
        return renderer;
    }
}

}

class SSAOCheckBox : public kvs::CheckBox
{
private:
    kvs::Scene* m_scene;

public:
    SSAOCheckBox( kvs::glut::Screen* screen ):
        kvs::CheckBox( screen ),
        m_scene( screen->scene() ) {}

    void stateChanged()
    {
        if ( state() )
        {
            const NoSSAORenderer* renderer = NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            if ( renderer )
            {
                const size_t repeats = renderer->repetitionLevel();
                m_scene->replaceRenderer( "Renderer", ::CreateRenderer( true, repeats ) );
            }
        }
        else
        {
            const SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            if ( renderer )
            {
                const size_t repeats = renderer->repetitionLevel();
                m_scene->replaceRenderer( "Renderer", ::CreateRenderer( false, repeats ) );
            }
        }
    }
};

class LODCheckBox : public kvs::CheckBox
{
private:
    kvs::Scene* m_scene;

public:
    LODCheckBox( kvs::glut::Screen* screen ):
        kvs::CheckBox( screen ),
        m_scene( screen->scene() ) {}

    void stateChanged()
    {
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( state() );
        }
        else
        {
            NoSSAORenderer* renderer = NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( state() );
        }
    }
};

class RepeatSlider : public kvs::Slider
{
private:
    kvs::Scene* m_scene;

public:
    RepeatSlider( kvs::glut::Screen* screen ):
        kvs::Slider( screen ),
        m_scene( screen->scene() ) {}

    void sliderMoved()
    {
        setCaption( "Repeats: " + kvs::String::ToString( int( value() ) ) );
    }

    void sliderReleased()
    {
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( size_t( value() ) );
        }
        else
        {
            NoSSAORenderer* renderer = NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( size_t( value() ) );
        }
    }
};

class SamplingRadiusSlider : public kvs::Slider
{
private:
    kvs::Scene* m_scene;

public:
    SamplingRadiusSlider( kvs::glut::Screen* screen ):
        kvs::Slider( screen ),
        m_scene( screen->scene() ) {}

    void sliderMoved()
    {
        ::SamplingRadius = current_value();
        setCaption( "Radius: " + kvs::String::ToString( ::SamplingRadius ) );
    }

    void sliderReleased()
    {
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            const size_t repeats = renderer->repetitionLevel();
            m_scene->replaceRenderer( "Renderer", ::CreateRenderer( true, repeats ) );
        }
    }

private:
    float current_value()
    {
        const float v = int( value() * 2 ) * 0.5;
        return kvs::Math::Clamp( v, minValue(), maxValue() );
    }
};

class SamplingPointsSlider : public kvs::Slider
{
private:
    kvs::Scene* m_scene;

public:
    SamplingPointsSlider( kvs::glut::Screen* screen ):
        kvs::Slider( screen ),
        m_scene( screen->scene() ) {}

    void sliderMoved()
    {
        ::SamplingPoints = int( value() );
        setCaption( "Points: " + kvs::String::ToString( ::SamplingPoints ) );
    }

    void sliderReleased()
    {
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            const size_t repeats = renderer->repetitionLevel();
            m_scene->replaceRenderer( "Renderer", ::CreateRenderer( true, repeats ) );
        }
    }
};

class OpacitySlider : public kvs::Slider
{
private:
    kvs::Scene* m_scene;

public:
    OpacitySlider( kvs::glut::Screen* screen ):
        kvs::Slider( screen ),
        m_scene( screen->scene() ) {}

    void sliderMoved()
    {
        ::Opacity = value();
        setCaption( "Opacity: " + kvs::String::ToString( int( ::Opacity * 100.0 ) * 0.01 ) );
    }

    void sliderReleased()
    {
        kvs::PolygonObject* object = new kvs::PolygonObject();
        object->shallowCopy( *kvs::PolygonObject::DownCast( m_scene->object( "Object" ) ) );
        object->setOpacity( kvs::Math::Round( value() * 255 ) );
        m_scene->replaceObject( "Object", object );
    }
};

class KeyPressEvent : public kvs::KeyPressEventListener
{
private:
    std::vector<kvs::WidgetBase*> m_widgets;

public:
    void addWidget( kvs::WidgetBase* widget )
    {
        m_widgets.push_back( widget );
    }

    void update( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        case kvs::Key::i:
        {
            for ( size_t i = 0; i < m_widgets.size(); i++ )
            {
                if ( m_widgets[i]->isShown() ) { m_widgets[i]->hide(); }
                else { m_widgets[i]->show(); }
            }
            break;
        }
        default: break;
        }
    }
};

int main( int argc, char** argv )
{
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::ShaderSource::AddSearchPath("../../../StochasticStreamline/Lib");

    // Application and screen.
    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle( "SSAOStochasticStylizedLineRenderer" );
    screen.show();

    // Input variables.
    const std::string filename = argv[1];
    const bool ssao = true;
    const bool lod = true;
    const size_t repeats = 50;

    // Visualization pipeline.
    kvs::PolygonObject* object = ::ImportPolygonObject( filename );
    object->setOpacity( kvs::Math::Clamp( int( ::Opacity * 255.0 ), 0, 255 ) );
    screen.registerObject( object, ::CreateRenderer( ssao, repeats ) );

    // Widgets.
    SSAOCheckBox ssao_check_box( &screen );
    ssao_check_box.setCaption( "SSAO" );
    ssao_check_box.setState( ssao );
    ssao_check_box.setMargin( 10 );
    ssao_check_box.show();

    LODCheckBox lod_check_box( &screen );
    lod_check_box.setCaption( "LOD" );
    lod_check_box.setState( lod );
    lod_check_box.setMargin( 10 );
    lod_check_box.setPosition( ssao_check_box.x(), ssao_check_box.y() + ssao_check_box.height() - 10 );
    lod_check_box.show();

    RepeatSlider repeat_slider( &screen );
    repeat_slider.setCaption( "Repeats: " + kvs::String::ToString( repeats ) );
    repeat_slider.setValue( repeats );
    repeat_slider.setRange( 1, 100 );
    repeat_slider.setMargin( 10 );
    repeat_slider.setPosition( lod_check_box.x(), lod_check_box.y() + lod_check_box.height() );
    repeat_slider.show();

    const float radius = 0.5;
    SamplingRadiusSlider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( radius ) );
    radius_slider.setValue( radius );
    radius_slider.setRange( 0.1, 5.0 );
    radius_slider.setMargin( 10 );
    radius_slider.setPosition( repeat_slider.x(), repeat_slider.y() + repeat_slider.height() - 10 );
    radius_slider.show();

    const int npoints = 256;
    SamplingPointsSlider points_slider( &screen );
    points_slider.setCaption( "Points: " + kvs::String::ToString( npoints ) );
    points_slider.setValue( npoints );
    points_slider.setRange( 1, 256 );
    points_slider.setMargin( 10 );
    points_slider.setPosition( radius_slider.x(), radius_slider.y() + radius_slider.height() - 10 );
    points_slider.show();

    const float opacity = 0.5f;
    OpacitySlider opacity_slider( &screen );
    opacity_slider.setCaption( "Opacity: " + kvs::String::ToString( opacity ) );
    opacity_slider.setValue( opacity );
    opacity_slider.setRange( 0, 1 );
    opacity_slider.setMargin( 10 );
    opacity_slider.setPosition( points_slider.x(), points_slider.y() + points_slider.height() - 10 );
    opacity_slider.show();

    // Events.
    KeyPressEvent key_event;
    key_event.addWidget( &ssao_check_box );
    key_event.addWidget( &lod_check_box );
    key_event.addWidget( &repeat_slider );
    key_event.addWidget( &radius_slider );
    key_event.addWidget( &points_slider );
    key_event.addWidget( &opacity_slider );
    screen.addEvent( &key_event );

    kvs::ScreenCaptureEvent capture_event;
    screen.addEvent( &capture_event );

    kvs::TargetChangeEvent target_change_event;
    screen.addEvent( &target_change_event );

    return app.run();
}
