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

// Stochastic renderers.
#include <StochasticStreamline/Lib/Streamline.h>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>


namespace
{
float SamplingRadius = 0.5f;
size_t SamplingPoints = 256;
}

namespace
{

inline kvs::StructuredVolumeObject* ImportVolumeObject(
    const std::string& filename,
    const float scale )
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );

    kvs::ValueArray<float> values = volume->values().asValueArray<float>();
    for ( size_t i = 0; i < values.size(); i++ ) { values[i] *= scale; }
    volume->setValues( values );
    volume->updateMinMaxValues();

    return volume;
}

inline kvs::PointObject* GenerateSeedPoints(
    const kvs::Vec3i min_coord,
    const kvs::Vec3i max_coord,
    const kvs::Vec3i stride )
{
    std::vector<kvs::Real32> v;
    for ( int k = min_coord.z(); k < max_coord.z(); k += stride.z() )
    {
        for ( int j = min_coord.y(); j < max_coord.y(); j += stride.y() )
        {
            for ( int i = min_coord.x(); i < max_coord.x(); i += stride.x() )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::PointObject* point = new kvs::PointObject;
    point->setCoords( kvs::ValueArray<kvs::Real32>( v ) );
    return point;
}

inline kvs::LineObject* ExtractStreamlines(
    const kvs::StructuredVolumeObject* volume,
    const kvs::PointObject* seed_points,
    const kvs::TransferFunction& tfunc )
{
    typedef StochasticStreamline::Streamline Mapper;
    Mapper* mapper = new Mapper();
    mapper->setSeedPoints( seed_points );
    mapper->setIntegrationInterval( 0.1 );
    mapper->setIntegrationMethod( Mapper::RungeKutta4th );
    mapper->setIntegrationDirection( Mapper::ForwardDirection );
    mapper->setTransferFunction( tfunc );
    return mapper->exec( volume );
}

inline kvs::RendererBase* CreateRenderer(
    const size_t repeats,
    const kvs::TransferFunction& tfunc,
    const bool ssao )
{
    if ( ssao )
    {
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;
        Renderer* renderer = new Renderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( tfunc );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( repeats );
        renderer->setEnabledLODControl( true );
        renderer->enableShading();
        renderer->setSamplingSphereRadius( ::SamplingRadius );
        renderer->setNumberOfSamplingPoints( ::SamplingPoints );
        return renderer;
    }
    else
    {
        typedef StochasticStreamline::StochasticTubeRenderer Renderer;
        Renderer* renderer = new Renderer();
        renderer->setName( "Renderer" );
        renderer->setTransferFunction( tfunc );
        renderer->setShader( kvs::Shader::BlinnPhong() );
        renderer->setRepetitionLevel( repeats );
        renderer->setEnabledLODControl( true );
        renderer->enableShading();
        return renderer;
    }
}

}

class OrientationAxis : public kvs::OrientationAxis
{
public:
    OrientationAxis( kvs::glut::Screen* screen ):
        kvs::OrientationAxis( screen, screen->scene() ) {}

    void screenResized()
    {
        setPosition( 0, screen()->height() - height() );
    }
};

class ColorMapBar : public kvs::ColorMapBar
{
public:
    ColorMapBar( kvs::glut::Screen* screen ):
        kvs::ColorMapBar( screen ) {}

    void screenResized()
    {
        setPosition( screen()->width() - width(), screen()->height() - height() );
    }
};

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
            typedef StochasticStreamline::StochasticTubeRenderer Renderer;
            const Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
            if ( renderer )
            {
                const size_t repeats = renderer->repetitionLevel();
                const kvs::TransferFunction& tfunc = renderer->transferFunction();
                m_scene->replaceRenderer( "Renderer", ::CreateRenderer( repeats, tfunc, true ) );
            }
        }
        else
        {
            typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer Renderer;
            const Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
            if ( renderer )
            {
                const size_t repeats = renderer->repetitionLevel();
                const kvs::TransferFunction& tfunc = renderer->transferFunction();
                m_scene->replaceRenderer( "Renderer", ::CreateRenderer( repeats, tfunc, false ) );
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
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            typedef SSAORenderer Renderer;
            Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( state() );
        }
        else
        {
            typedef StochasticStreamline::StochasticTubeRenderer Renderer;
            Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
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
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            typedef SSAORenderer Renderer;
            Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( size_t( value() ) );
        }
        else
        {
            typedef StochasticStreamline::StochasticTubeRenderer Renderer;
            Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
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
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            typedef SSAORenderer Renderer;
            Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
            const size_t repeats = renderer->repetitionLevel();
            const kvs::TransferFunction& tfunc = renderer->transferFunction();
            m_scene->replaceRenderer( "Renderer", ::CreateRenderer( repeats, tfunc, true ) );
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
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
        const bool ssao = ( SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            typedef SSAORenderer Renderer;
            Renderer* renderer = Renderer::DownCast( m_scene->renderer( "Renderer" ) );
            const size_t repeats = renderer->repetitionLevel();
            const kvs::TransferFunction& tfunc = renderer->transferFunction();
            m_scene->replaceRenderer( "Renderer", ::CreateRenderer( repeats, tfunc, true ) );
        }
    }
};

class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{
private:
    ColorMapBar* m_cmap_bar;

public:
    TransferFunctionEditor( kvs::glut::Screen* screen, ColorMapBar* cmap_bar ):
        kvs::glut::TransferFunctionEditor( screen ),
        m_cmap_bar( cmap_bar ) {}

    void apply()
    {
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        typedef AmbientOcclusionRendering::SSAOStochasticTubeRenderer SSAORenderer;
        const bool ssao = ( SSAORenderer::DownCast( scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            typedef SSAORenderer Renderer;
            Renderer* renderer = Renderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( transferFunction() );
        }
        else
        {
            typedef StochasticStreamline::StochasticTubeRenderer Renderer;
            Renderer* renderer = Renderer::DownCast( scene->renderer( "Renderer" ) );
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

    // Application and screen.
    kvs::glut::Application app( argc, argv );
    kvs::glut::Screen screen( &app );
    screen.setTitle( "Stochastic Tubeline Rendering with SSAO" );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.show();

    // Input variables.
    const std::string filename = argv[1];
    const bool ssao = true;
    const bool lod = true;
    const float scale = 100.0f;
    const size_t repeats = 40;
    const kvs::Vec3i min_coord( 0, 0, 0 );
    const kvs::Vec3i max_coord( 250, 250, 250 );
    const kvs::Vec3i stride( 30, 30, 30 );
    const kvs::TransferFunction tfunc( kvs::DivergingColorMap::CoolWarm( 256 ) );

    // Visualization pipeline.
    kvs::StructuredVolumeObject* volume = ::ImportVolumeObject( filename, scale);
    kvs::PointObject* seed_points = ::GenerateSeedPoints( min_coord, max_coord, stride );
    kvs::LineObject* object = ::ExtractStreamlines( volume, seed_points, tfunc );
    kvs::RendererBase* renderer = ::CreateRenderer( repeats, tfunc, ssao );
    screen.registerObject( object, renderer );
    delete seed_points;

    // Widgets.
    OrientationAxis orientation_axis( &screen );
    orientation_axis.setBoxType( kvs::OrientationAxis::SolidBox );
    orientation_axis.show();

    ColorMapBar cmap_bar( &screen );
    cmap_bar.setCaption( "Velocity Magnitude" );
    cmap_bar.setColorMap( tfunc.colorMap() );
    cmap_bar.setRange( volume->minValue(), volume->maxValue() );
    cmap_bar.show();

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

    TransferFunctionEditor editor( &screen, &cmap_bar );
    editor.setTransferFunction( tfunc );
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
