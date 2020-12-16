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
#include <AmbientOcclusionRendering/Lib/SSAOStochasticTubeRenderer.h>
#include <kvs/StochasticUniformGridRenderer>
#include <AmbientOcclusionRendering/Lib/SSAOStochasticUniformGridRenderer.h>
// Other
#include <kvs/DivergingColorMap>
#include <kvs/StochasticRenderingCompositor>
#include "Streamline.h"
#include <kvs/ObjectManager>
#include <kvs/RendererManager>
#include <kvs/IDManager>


/*===========================================================================*/
/**
 *  @brief  Uniform Grid Model class manages SSAO parameters
 */
/*===========================================================================*/
struct UniformGridModel
{
    using SSAORenderer = AmbientOcclusionRendering::SSAOStochasticUniformGridRenderer;
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
    using SSAORenderer = AmbientOcclusionRendering::SSAOStochasticTubeRenderer;
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
    kvs::StochasticRenderingCompositor compositor( screen.scene() );
    compositor.setRepetitionLevel( 1 );
    compositor.enableLODControl();
    screen.setEvent( &compositor );

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

    kvs::CheckBox ssao_check_box( &screen );
    ssao_check_box.setCaption( "SSAO" );
    ssao_check_box.setState( uniform_model.ssao );
    ssao_check_box.setMargin( 10 );
    ssao_check_box.anchorToTopLeft();
    ssao_check_box.show();
    ssao_check_box.stateChanged( [&] ()
    {
        uniform_model.ssao = ssao_check_box.state();
	tube_model.ssao = ssao_check_box.state();
        screen.scene()->replaceRenderer( "UniformRenderer", uniform_model.renderer() );
	screen.scene()->replaceRenderer( "TubeRenderer", tube_model.renderer() );
    } );

    kvs::CheckBox checkbox( &screen );
    checkbox.setCaption( "LOD" );
    checkbox.setMargin( 10 );
    checkbox.setState( true );
    checkbox.anchorToBottom( &ssao_check_box );
    checkbox.stateChanged(
        [&]() {
            compositor.setEnabledLODControl( checkbox.state() );
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
            compositor.setRepetitionLevel( int( repetition.value() + 0.5 ) );
            screen.redraw();
        } );
    repetition.show();

    kvs::KeyPressEventListener h_key;
    h_key.update( [&] ( kvs::KeyEvent* event )
    {
        switch( event->key() )
        {
        case kvs::Key::h:
        {
            if ( checkbox.isVisible() )
            {
	        ssao_check_box.hide();
                checkbox.hide();
                repetition.hide();
            }
            else
            {
	        ssao_check_box.show();
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
                
    return app.run();
}
