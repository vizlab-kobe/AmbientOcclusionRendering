#include <vector>
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/glut/TransferFunctionEditor>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/LineObject>
#include <kvs/DivergingColorMap>
#include <kvs/ShaderSource>
#include <kvs/OrientationAxis>
#include <kvs/ColorMapBar>
#include <kvs/CheckBox>
#include <kvs/CheckBoxGroup>
#include <kvs/Slider>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/KeyPressEventListener>
#include "Input.h"

// Stochastic renderers.
#include <StochasticStreamline/Lib/Streamline.h>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>
typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
typedef StochasticStreamline::StochasticTubeRenderer NoSSAORenderer;


namespace
{

inline kvs::StructuredVolumeObject* Import( const local::Input& input )
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( input.filename );

    kvs::ValueArray<float> values = volume->values().asValueArray<float>();
    for ( size_t i = 0; i < values.size(); i++ ) { values[i] *= input.scale; }
    volume->setValues( values );
    volume->updateMinMaxValues();

    return volume;
}

inline kvs::LineObject* Streamline( const local::Input& input, const kvs::StructuredVolumeObject* volume )
{
    std::vector<kvs::Real32> v;
    for ( int k = input.min_coord.z(); k < input.max_coord.z(); k += input.stride.z() )
    {
        for ( int j = input.min_coord.y(); j < input.max_coord.y(); j += input.stride.y() )
        {
            for ( int i = input.min_coord.x(); i < input.max_coord.x(); i += input.stride.x() )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::SharedPointer<kvs::PointObject> seeds( new kvs::PointObject );
    seeds->setCoords( kvs::ValueArray<kvs::Real32>( v ) );

    typedef StochasticStreamline::Streamline Mapper;
    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( seeds.get() );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    mapper->setTransferFunction( input.tfunc );
    return mapper->exec( volume );
}


inline kvs::RendererBase* Renderer( const local::Input& input )
{
    if ( input.ssao )
    {
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;
        Renderer* renderer = new Renderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( input.tfunc );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( input.repeats );
        renderer->setEnabledLODControl( input.lod );
        renderer->setSamplingSphereRadius( input.radius );
        renderer->setNumberOfSamplingPoints( input.points );
        renderer->enableShading();
        return renderer;
    }
    else
    {
        typedef StochasticStreamline::StochasticTubeRenderer Renderer;
        Renderer* renderer = new Renderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( input.tfunc );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( input.repeats );
        renderer->setEnabledLODControl( input.lod );
        renderer->enableShading();
        return renderer;
    }
}

}

class SSAOCheckBox : public kvs::CheckBox
{
private:
    kvs::Scene* m_scene;
    local::Input& m_input;

public:
    SSAOCheckBox( kvs::glut::Screen& screen, local::Input& input ):
        kvs::CheckBox( &screen ),
        m_scene( screen.scene() ),
        m_input( input )
    {
        setCaption( "SSAO" );
        setState( input.ssao );
        setMargin( 10 );
    }

    void stateChanged()
    {
        if ( state() )
        {
            const NoSSAORenderer* renderer = NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.ssao = true;
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
        }
        else
        {
            const SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.ssao = false;
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
        }
        m_scene->replaceRenderer( "Renderer", ::Renderer( m_input ) );
    }
};

class LODCheckBox : public kvs::CheckBox
{
private:
    kvs::Scene* m_scene;

public:
    LODCheckBox( kvs::glut::Screen& screen, local::Input& input ):
        kvs::CheckBox( &screen ),
        m_scene( screen.scene() )
    {
        setCaption( "LOD" );
        setState( input.lod );
        setMargin( 10 );
    }

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
    RepeatSlider( kvs::glut::Screen& screen, local::Input& input ):
        kvs::Slider( &screen ),
        m_scene( screen.scene() )
    {
        setCaption( "Repeats: " + kvs::String::ToString( input.repeats ) );
        setValue( input.repeats );
        setRange( 1, 100 );
        setMargin( 10 );
    }

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
    local::Input& m_input;

public:
    SamplingRadiusSlider( kvs::glut::Screen& screen, local::Input& input ):
        kvs::Slider( &screen ),
        m_scene( screen.scene() ),
        m_input( input )
    {
        setCaption( "Radius: " + kvs::String::ToString( input.radius ) );
        setValue( input.radius );
        setRange( 0.1, 5.0 );
        setMargin( 10 );
    }

    void sliderMoved()
    {
        m_input.radius = current_value();
        setCaption( "Radius: " + kvs::String::ToString( m_input.radius ) );
    }

    void sliderReleased()
    {
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            const SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
            m_scene->replaceRenderer( "Renderer", ::Renderer( m_input ) );
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
    local::Input& m_input;

public:
    SamplingPointsSlider( kvs::glut::Screen& screen, local::Input& input ):
        kvs::Slider( &screen ),
        m_scene( screen.scene() ),
        m_input( input )
    {
        setCaption( "Points: " + kvs::String::ToString( input.points ) );
        setValue( input.points );
        setRange( 1, 256 );
        setMargin( 10 );
    }

    void sliderMoved()
    {
        m_input.points = int( value() );
        setCaption( "Points: " + kvs::String::ToString( m_input.points ) );
    }

    void sliderReleased()
    {
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            SSAORenderer* renderer = SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
            m_scene->replaceRenderer( "Renderer", ::Renderer( m_input ) );
        }
    }
};

class OrientationAxis : public kvs::OrientationAxis
{
public:
    OrientationAxis( kvs::glut::Screen& screen ):
        kvs::OrientationAxis( &screen, screen.scene() ) {}

    void screenResized()
    {
        setPosition( 0, screen()->height() - height() );
    }
};

class ColorMapBar : public kvs::ColorMapBar
{
public:
    ColorMapBar( kvs::glut::Screen& screen ):
        kvs::ColorMapBar( &screen ) {}

    void screenResized()
    {
        setPosition( screen()->width() - width(), screen()->height() - height() );
    }
};

class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{
private:
    ColorMapBar* m_cmap_bar;

public:
    TransferFunctionEditor( kvs::glut::Screen& screen, ColorMapBar* cmap_bar ):
        kvs::glut::TransferFunctionEditor( &screen ),
        m_cmap_bar( cmap_bar ) {}

    void apply()
    {
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        const bool ssao = ( SSAORenderer::DownCast( scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            SSAORenderer* renderer = SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( transferFunction() );
        }
        else
        {
            NoSSAORenderer* renderer = NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( transferFunction() );
        }
        m_cmap_bar->setColorMap( colorMap() );
        screen()->redraw();
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
    kvs::StructuredVolumeObject* volume = ::Import( input );
    kvs::LineObject* object = ::Streamline( input, volume );
    kvs::RendererBase* renderer = ::Renderer( input );
    screen.registerObject( object, renderer );

    // Widgets.
    SSAOCheckBox ssao_check_box( screen, input );
    ssao_check_box.show();

    LODCheckBox lod_check_box( screen, input );
    lod_check_box.setPosition( ssao_check_box.x(), ssao_check_box.y() + ssao_check_box.height() - 10 );
    lod_check_box.show();

    RepeatSlider repeat_slider( screen, input );
    repeat_slider.setPosition( lod_check_box.x(), lod_check_box.y() + lod_check_box.height() );
    repeat_slider.show();

    SamplingRadiusSlider radius_slider( screen, input );
    radius_slider.setPosition( repeat_slider.x(), repeat_slider.y() + repeat_slider.height() - 10 );
    radius_slider.show();

    SamplingPointsSlider points_slider( screen, input );
    points_slider.setPosition( radius_slider.x(), radius_slider.y() + radius_slider.height() - 10 );
    points_slider.show();

    OrientationAxis orientation_axis( screen );
    orientation_axis.setBoxType( kvs::OrientationAxis::SolidBox );
    orientation_axis.show();

    ColorMapBar cmap_bar( screen );
    cmap_bar.setCaption( "Velocity Magnitude" );
    cmap_bar.setColorMap( input.tfunc.colorMap() );
    cmap_bar.setRange( volume->minValue(), volume->maxValue() );
    cmap_bar.show();

    TransferFunctionEditor editor( screen, &cmap_bar );
    editor.setTransferFunction( input.tfunc );
    editor.setVolumeObject( volume );
    editor.show();

    // Events.
    KeyPressEvent key_event;
    key_event.addWidget( &orientation_axis );
    key_event.addWidget( &cmap_bar );
    key_event.addWidget( &ssao_check_box );
    key_event.addWidget( &lod_check_box );
    key_event.addWidget( &repeat_slider );
    key_event.addWidget( &radius_slider );
    key_event.addWidget( &points_slider );
    screen.addEvent( &key_event );

    kvs::ScreenCaptureEvent capture_event;
    screen.addEvent( &capture_event );

    kvs::TargetChangeEvent target_change_event;
    screen.addEvent( &target_change_event );

    return app.run();
}
