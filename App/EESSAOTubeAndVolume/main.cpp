#include <kvs/Application>
#include <kvs/Screen>
// Widget
#include <kvs/TransferFunctionEditor>
#include <kvs/Slider>
#include <kvs/CheckBox>
#include <kvs/PolygonObject>
#include <kvs/TargetChangeEvent>
#include <kvs/ScreenCaptureEvent>
#include <kvs/KeyPressEventListener>
// Visualization pipeline module
#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVectorToScalar>
#include <kvs/StructuredVolumeObject>
#include <StochasticStreamline/Lib/StochasticTubeRenderer.h>
#include "SSAOStochasticTubeRenderer.h"
#include <kvs/StochasticUniformGridRenderer>
#include "SSAOStochasticUniformGridRenderer.h"
// Other
#include <kvs/DivergingColorMap>
#include <kvs/StochasticRenderingCompositor>
#include "StochasticRenderingCompositor.h"
#include "Streamline.h"
#include <kvs/ObjectManager>
#include <kvs/RendererManager>
#include <kvs/IDManager>
#include <kvs/PaintEventListener>


/*===========================================================================*/
/**
 *  @brief  Uniform Grid Model class manages SSAO parameters
 */
/*===========================================================================*/
struct UniformGridModel
{
    using SSAORenderer = local::SSAOStochasticUniformGridRenderer;
    using Renderer = kvs::StochasticUniformGridRenderer;

    bool ssao; ///< SSAO flag
    bool lod; ///< LoD flag
    size_t repeats; ///< number of repetitions for stochasti rendering
    float radius; ///< radius of point sampling region for SSAO
    int points; ///< number of points used for SSAO
    kvs::TransferFunction tfunc; ///< transfer function

    kvs::StructuredVolumeObject* import( const std::string& filename )
    {
        kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
	kvs::StructuredVolumeObject* scalar_volume = new kvs::StructuredVectorToScalar( volume );
	scalar_volume->setName( "UniformGrid" );
        return scalar_volume;
    }

    kvs::RendererBase* renderer()
    {
        //kvs::Light::SetModelTwoSide( true );
        if ( ssao )
        {
            auto* renderer = new SSAORenderer();
            renderer->setName( "UniformRenderer" );
            renderer->setTransferFunction( tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( repeats );
            renderer->setEnabledLODControl( lod );
            renderer->enableShading();
            renderer->setSamplingSphereRadius( radius );
            renderer->setNumberOfSamplingPoints( points );
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "UniformRenderer" );
            renderer->setTransferFunction( tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( repeats );
            renderer->setEnabledLODControl( lod );
            renderer->enableShading();
            return renderer;
        }
    }
};

/*===========================================================================*/
/**
 *  @brief  Generate seed points for mapping streamline.
 */
/*===========================================================================*/
kvs::PointObject* generateSeedPoints( kvs::StructuredVolumeObject* volume, kvs::Vec3i stride )
{
    std::vector<kvs::Real32> v;
    for ( int k = volume->minObjectCoord().z(); k <= volume->maxObjectCoord().z(); k += stride.z() )
    {
        for ( int j = volume->minObjectCoord().y(); j <= volume->maxObjectCoord().y(); j += stride.y() )
        {
            for ( int i = volume->minObjectCoord().x(); i <= volume->maxObjectCoord().x(); i += stride.x() )
            {
                v.push_back( static_cast<kvs::Real32>(i) );
                v.push_back( static_cast<kvs::Real32>(j) );
                v.push_back( static_cast<kvs::Real32>(k) );
            }
        }
    }

    kvs::PointObject* seeds = new kvs::PointObject;
    seeds->setCoords( kvs::ValueArray<kvs::Real32>( v ) );
    return seeds;
}

/*===========================================================================*/
/**
 *  @brief  Tube Model class manages SSAO parameters
 */
/*===========================================================================*/
struct TubeModel
{
    using SSAORenderer = local::SSAOStochasticTubeRenderer;
    using Renderer = StochasticStreamline::StochasticTubeRenderer;

    bool ssao; ///< SSAO flag
    bool lod; ///< LoD flag
    size_t repeats; ///< number of repetitions for stochasti rendering
    float radius; ///< radius of point sampling region for SSAO
    int points; ///< number of points used for SSAO
    kvs::TransferFunction tfunc; ///< transfer function

    kvs::LineObject* import( const std::string& filename )
    {
        kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( filename );
        kvs::ValueArray<float> values = volume->values().asValueArray<float>();
        for ( size_t i = 0; i < values.size(); i++ ) { values[i] *= 100.0; }
        volume->setValues( values );
        volume->updateMinMaxValues();
        kvs::Vec3i stride( 30, 30 ,30 );
        kvs::PointObject* seeds = generateSeedPoints( volume, stride );
        typedef local::Streamline Mapper;
        Mapper* mapper = new Mapper();
        mapper->setSeedPoints( seeds );
        mapper->setIntegrationInterval( 0.1 );
        mapper->setIntegrationMethod( Mapper::RungeKutta4th );
        mapper->setIntegrationDirection( Mapper::ForwardDirection );
        mapper->setTransferFunction( tfunc );
        return mapper->exec( volume ); 
    }

    kvs::RendererBase* renderer()
    {
        //kvs::Light::SetModelTwoSide( true );
        if ( ssao )
        {
            auto* renderer = new SSAORenderer();
            renderer->setName( "TubeRenderer" );
            renderer->setTransferFunction( tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( repeats );
            renderer->setEnabledLODControl( lod );
            renderer->enableShading();
            renderer->setSamplingSphereRadius( radius );
            renderer->setNumberOfSamplingPoints( points );
            return renderer;
        }
        else
        {
            auto* renderer = new Renderer();
            renderer->setName( "TubeRenderer" );
            renderer->setTransferFunction( tfunc );
            renderer->setShader( kvs::Shader::BlinnPhong() );
            renderer->setRepetitionLevel( repeats );
            renderer->setEnabledLODControl( lod );
            renderer->enableShading();
            return renderer;
        }
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
    // Shader path.
    kvs::ShaderSource::AddSearchPath("../../Lib");
    kvs::ShaderSource::AddSearchPath("../../../StochasticStreamline/Lib");

    // Application and screen.
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setTitle("SSAOStochasticRenderingCompositor Streamline and UniformGrid");
    screen.setSize( 1024, 1024 );
    screen.show();

    // Parameters.
    UniformGridModel uniform_model;
    uniform_model.ssao = true;
    uniform_model.lod = true;
    uniform_model.repeats = 1;
    uniform_model.radius = 0.5;
    uniform_model.points = 256;
    uniform_model.tfunc = kvs::TransferFunction( kvs::ColorMap::BrewerSpectral( 256 ) );

    TubeModel tube_model;
    tube_model.ssao = true;
    tube_model.lod = true;
    tube_model.repeats = 1;
    tube_model.radius = 0.5;
    tube_model.points = 256;
    tube_model.tfunc = kvs::TransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    
    // Visualization pipeline.
    const std::string magnetic_file = argv[1];
    const std::string velocity_file = argv[2];
    screen.registerObject( uniform_model.import( magnetic_file ), uniform_model.renderer() );
    screen.registerObject( tube_model.import( velocity_file ), tube_model.renderer() );

    // StochasticRenderingCompositor
    local::StochasticRenderingCompositor ssao_compositor( screen.scene() );
    ssao_compositor.setRepetitionLevel( 1 );
    ssao_compositor.enableLODControl();
    ssao_compositor.setShader( kvs::Shader::BlinnPhong() );
    screen.setEvent( &ssao_compositor );

    // Widgets.
    kvs::TransferFunctionEditor editor1( &screen );
    editor1.setPosition( screen.x() + screen.width(), screen.y() );
    editor1.setVolumeObject( kvs::StructuredVolumeObject::DownCast( screen.scene()->object( "UniformGrid" ) ) );
    editor1.setTransferFunction( uniform_model.tfunc );
    editor1.show();
    editor1.apply(
        [&]( kvs::TransferFunction tfunc )
        {
            uniform_model.tfunc = tfunc;
            auto* scene = screen.scene();
            if ( uniform_model.ssao )
            {
                auto* renderer = UniformGridModel::SSAORenderer::DownCast( scene->renderer( "UniformRenderer" ) );
                renderer->setTransferFunction( uniform_model.tfunc );
            }
            else
            {
                auto* renderer = UniformGridModel::Renderer::DownCast( scene->renderer( "UniformRenderer" ) );
                renderer->setTransferFunction( uniform_model.tfunc );
            }
            screen.redraw();
        } );

    kvs::TransferFunctionEditor editor2( &screen );
    editor2.setPosition( screen.x() + screen.width(), screen.y() );
    editor2.setTransferFunction( tube_model.tfunc );
    editor2.show();
    editor2.apply( [&] ( kvs::TransferFunction tfunc )
    {
        tube_model.tfunc = tfunc;
        auto* scene = screen.scene();
        if ( tube_model.ssao )
        {
            auto* renderer = TubeModel::SSAORenderer::DownCast( scene->renderer( "TubeRenderer" ) );
            renderer->setTransferFunction( tfunc );
        }
        else
        {
            auto* renderer = TubeModel::Renderer::DownCast( scene->renderer( "TubeRenderer" ) );
            renderer->setTransferFunction( tfunc );
        }
        //m_bar.setColorMap( tfunc.colorMap() );
        screen.redraw();
    } );

    kvs::CheckBox checkbox( &screen );
    checkbox.setCaption( "LOD" );
    checkbox.setMargin( 10 );
    checkbox.setState( true );
    checkbox.anchorToTopLeft();
    checkbox.stateChanged(
        [&]() {
            ssao_compositor.setEnabledLODControl( checkbox.state() );
            screen.redraw();
        } );
    checkbox.show();

    kvs::Slider repetition( &screen );
    repetition.setCaption( "Repetition" );
    repetition.setWidth( 150 );
    repetition.setMargin( 10 );
    repetition.setValue( uniform_model.repeats );
    repetition.setRange( 1, 100 );
    repetition.anchorToBottom( &checkbox );
    repetition.valueChanged(
        [&]() {
            ssao_compositor.setRepetitionLevel( int( repetition.value() + 0.5 ) );
            screen.redraw();
        } );
    repetition.show();

    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( uniform_model.radius ) );
    radius_slider.setValue( 0.5 );
    radius_slider.setRange( 0.01, 5.0 );
    radius_slider.setMargin( 10 );
    radius_slider.anchorToBottom( &repetition );
    radius_slider.show();
    radius_slider.sliderMoved( [&] ()
    {
        const float min_value = radius_slider.minValue();
        const float max_value = radius_slider.maxValue();
        const float v = int( radius_slider.value() * 10 ) * 0.1f;
        uniform_model.radius = kvs::Math::Clamp( v, min_value, max_value );
        radius_slider.setCaption( "Radius: " + kvs::String::From( uniform_model.radius ) );
    } );
    radius_slider.sliderReleased( [&] ()
    {
        ssao_compositor.setSamplingSphereRadius( uniform_model.radius );
        screen.redraw();
    } );

    int nsamples = 256;
    kvs::Slider nsample_slider( &screen );
    nsample_slider.setCaption( "Nsample: " + kvs::String::ToString( nsamples ) );
    nsample_slider.setValue( nsamples );
    nsample_slider.setRange( 1, 256 );
    nsample_slider.setMargin( 10 );
    nsample_slider.anchorToBottom( &radius_slider );
    nsample_slider.show();
    nsample_slider.sliderMoved( [&] ()
    {
        const int min_value = nsample_slider.minValue();
        const int max_value = nsample_slider.maxValue();
        const int v = int( nsample_slider.value() );
        nsamples = kvs::Math::Clamp( v, min_value, max_value );
        nsample_slider.setCaption( "Nsample: " + kvs::String::From( nsamples ) );
    } );
    nsample_slider.sliderReleased( [&] ()
    {
        ssao_compositor.setNumberOfSamplingPoints( nsamples );
        screen.redraw();
    } );

    float edge = 1.0;
    kvs::Slider edge_slider( &screen );
    edge_slider.setCaption( "Edge: " + kvs::String::ToString( edge ) );
    edge_slider.setValue( edge );
    edge_slider.setRange( 0.0, 5.0 );
    edge_slider.setMargin( 10 );
    edge_slider.anchorToBottom( &nsample_slider );
    edge_slider.show();
    edge_slider.sliderMoved( [&] ()
    {
        edge = int( edge_slider.value() * 10 ) * 0.1f;
        edge_slider.setCaption( "Edge: " + kvs::String::From( edge ) );
    } );
    edge_slider.sliderReleased( [&] ()
    {
      auto* scene = screen.scene();
      auto* renderer = local::SSAOStochasticUniformGridRenderer::DownCast( scene->renderer( "UniformRenderer" ) );
      renderer->setEdgeFactor( edge );
    } );

    kvs::KeyPressEventListener h_key;
    h_key.update( [&] ( kvs::KeyEvent* event )
    {
        switch( event->key() )
        {
        case kvs::Key::h:
        {
            if ( checkbox.isVisible() )
            {
                checkbox.hide();
                repetition.hide();
            }
            else
            {
                checkbox.show();
                repetition.show();
            }
        }
        default: break;
        }
    } );

    kvs::ScreenCaptureEvent capture_event;
    
    screen.addEvent( &h_key );
    screen.addEvent( &capture_event );

    kvs::PaintEventListener time;
    time.update( [&] ()
    {
        static size_t counter = 1;
        static float time = 0.0f;

        time += ssao_compositor.timer().msec();
        if ( counter++ == 10 )
        {
            std::cout << "Rendering time: " << time / counter << " [msec]" << std::endl;
            counter = 1;
            time = 0.0f;
        }

        } );
    
    screen.addEvent( &time );
                
    return app.run();
}
