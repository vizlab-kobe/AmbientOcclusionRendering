#include <kvs/Application>
#include <kvs/Screen>
//Widget
#include <kvs/TransferFunctionEditor>
#include <kvs/Slider>
#include <kvs/CheckBox>
#include <kvs/OrientationAxis>
//Event
#include <kvs/KeyPressEventListener>
#include <kvs/TargetChangeEvent>
#include <kvs/ScreenCaptureEvent>
//Visualization pipeline module
#include <kvs/PolygonObject>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/DivergingColorMap>
#include "Streamline.h"
#include "SSAOStochasticPolygonRenderer.h"
#include "SSAOStochasticTubeRenderer.h"
#include <kvs/StochasticRenderingCompositor>
#include "StochasticRenderingCompositor.h"
#include <kvs/StructuredVectorToScalar>
#include <kvs/Isosurface>
#include <kvs/PolygonToPolygon>
#include <kvs/PaintEventListener>
#include <iostream>
#include <cmath>


kvs::Vec3i min_coord( 0, 0, 0 );
kvs::Vec3i max_coord( 250, 250, 250 );

kvs::PolygonObject* createBoundingSphere()
{
    float pi  = 3.14159;
    float radius = 125;
    int dim_theta = 18; // Number of latitude divisions
    int dim_phi = 36; // Number of longitude divisions
    float d_theta = pi / dim_theta;
    float d_phi = 2 * pi / dim_phi;

    float theta = 0, phi = 0; // 0 <= theta <= pi, 0 <= phi <= 2*pi
    
    kvs::ValueArray<kvs::Real32> coord( static_cast<size_t>( ( dim_theta + 1 ) * dim_phi * 3 ) );
    kvs::ValueArray<kvs::UInt32> connect( static_cast<size_t>( dim_theta * dim_phi * 3 * 2 ) );
    kvs::ValueArray<kvs::Real32> normal( static_cast<size_t>( dim_theta * dim_phi * 3 * 2 ) );

    int k = 0;
    for(int j = 0; k <= dim_theta; j += dim_phi * 3 )
    {
        phi = 0;
        
        for(int i = 0; phi < 2 * pi; i += 3 )
        {  
            coord[ i + j + 0 ] = radius * std::sin( theta ) * std::cos( phi ) + 125;
            coord[ i + j + 1 ] = radius * std::sin( theta ) * std::sin( phi ) + 125;
            coord[ i + j + 2 ] = radius * std::cos( theta ) + 125;
            
            phi += d_phi;
        }
        theta += d_theta;
        k++;
    }

    int point_index = 0;
    for( int i = 0; point_index < int( connect.size() ) / 6; i += 6 )
    {
        if( point_index % dim_phi != dim_phi - 1 )
        {
            connect[ i + 0 ] = point_index;
            connect[ i + 1 ] = point_index + dim_phi + 1;
            connect[ i + 2 ] = point_index + dim_phi;

            connect[ i + 3 ] = point_index;
            connect[ i + 4 ] = point_index + 1;
            connect[ i + 5 ] = point_index + dim_phi + 1;
        }
        else
        {
            connect[ i + 0 ] = point_index;
            connect[ i + 1 ] = point_index + 1;
            connect[ i + 2 ] = point_index + dim_phi;

            connect[ i + 3 ] = point_index;
            connect[ i + 4 ] = point_index - dim_phi + 1;
            connect[ i + 5 ] = point_index + 1;
        }
        
        point_index++;
    }

    point_index = 0;
    for( int i = 0; point_index < int( connect.size() ) / 6; i += 6 )
    {
        if( point_index % dim_phi != dim_phi - 1 )
        {
            int coord_index = point_index * 3;
            kvs::Vec3 v0( coord[ coord_index ],
                          coord[ coord_index + 1 ],
                          coord[ coord_index + 2 ] );
            kvs::Vec3 v1( coord[ coord_index + dim_phi * 3 + 3],
                          coord[ coord_index + dim_phi * 3 + 4 ],
                          coord[ coord_index + dim_phi * 3 + 5 ] );
            kvs::Vec3 v2( coord[ coord_index + dim_phi * 3 ],
                          coord[ coord_index + dim_phi * 3 + 1 ],
                          coord[ coord_index + dim_phi * 3 + 2 ] );

            kvs::Vec3 n;

            n = ( v1 - v0 ).cross( v2 - v0 );
            normal[ i + 0 ] = n.x();
            normal[ i + 1 ] = n.y();
            normal[ i + 2 ] = n.z();

            kvs::Vec3 v3( coord[ coord_index + 3 ],
                          coord[ coord_index + 4 ],
                          coord[ coord_index + 5 ] );
            kvs::Vec3 v4( coord[ coord_index + dim_phi * 3 + 3],
                          coord[ coord_index + dim_phi * 3 + 4 ],
                          coord[ coord_index + dim_phi * 3 + 5 ] );

            n = ( v3 - v0 ).cross( v4 - v0 );
            normal[ i + 3 ] = n.x();
            normal[ i + 4 ] = n.y();
            normal[ i + 5 ] = n.z();
        }
        else
        {
            int coord_index = point_index * 3;
            kvs::Vec3 v0( coord[ coord_index ],
                          coord[ coord_index + 1 ],
                          coord[ coord_index + 2 ] );
            kvs::Vec3 v1( coord[ coord_index + 3],
                          coord[ coord_index + 4 ],
                          coord[ coord_index + 5 ] );
            kvs::Vec3 v2( coord[ coord_index + dim_phi * 3 ],
                          coord[ coord_index + dim_phi * 3 + 1 ],
                          coord[ coord_index + dim_phi * 3 + 2 ] );

            kvs::Vec3 n;

            n = ( v1 - v0 ).cross( v2 - v0 );
            normal[ i + 0 ] = n.x();
            normal[ i + 1 ] = n.y();
            normal[ i + 2 ] = n.z();

            kvs::Vec3 v3( coord[ coord_index - dim_phi * 3 + 3 ],
                          coord[ coord_index - dim_phi * 3 + 4 ],
                          coord[ coord_index - dim_phi * 3 + 5 ] );
            kvs::Vec3 v4( coord[ coord_index + 3],
                          coord[ coord_index + 4 ],
                          coord[ coord_index + 5 ] );

            n = ( v3 - v0 ).cross( v4 - v0 );
            normal[ i + 3 ] = n.x();
            normal[ i + 4 ] = n.y();
            normal[ i + 5 ] = n.z();
        }

        point_index ++;
    }

    double opacity = 0.5;
    kvs::PolygonObject* polygon = new kvs::PolygonObject();
    polygon->setCoords( coord );
    polygon->setConnections( connect );
    polygon->setPolygonType( kvs::PolygonObject::Triangle );
    polygon->setNormalTypeToPolygon();
    polygon->setNormals( normal );
    polygon->setColor( kvs::RGBColor::Red() );
    polygon->setName( "Polygon" );

    kvs::PolygonObject* sphere = new kvs::PolygonToPolygon( polygon );
    sphere->setOpacity( kvs::Math::Clamp( int( opacity * 255.0 ), 0, 255 ) );
    sphere->setColor( kvs::RGBColor::White() );
    sphere->setName( "Polygon" );
    
    return sphere;
}

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

kvs::LineObject* createStreamline( std::string filename )
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
    mapper->setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    return mapper->exec( volume );
}

/*===========================================================================*/
/**
 *  @brief  Main function. Tube and polygon visualization.
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
    screen.setTitle("SSAOStochasticRenderingCompositor Streamline and Polygon");
    screen.setSize( 1024, 1024 );
    screen.show();

    // Import volume object.
    std::string velocity_file = argv[2];

    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( velocity_file );
    kvs::LineObject* streamline = createStreamline( velocity_file );
    kvs::PolygonObject* polygon = createBoundingSphere();
    
    // Declare SSAOStochasticPolygonRenderer
    local::SSAOStochasticPolygonRenderer* polygon_renderer = new local::SSAOStochasticPolygonRenderer();
    polygon_renderer->setName( "PolygonRenderer" );

    // Declare SSAOStochasticTubeRenderer
    local::SSAOStochasticTubeRenderer* tube_renderer = new local::SSAOStochasticTubeRenderer();
    tube_renderer->setName( "StochasticTubeRenderer" );
    tube_renderer->setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    
    // Register objects and renderers
    screen.registerObject( polygon, polygon_renderer );
    screen.registerObject( streamline, tube_renderer );

    // Declare StochasticRenderingCompositor.
    local::StochasticRenderingCompositor compositor( screen.scene() );
    compositor.setRepetitionLevel( 1 );
    compositor.enableLODControl();
    compositor.setShader( kvs::Shader::BlinnPhong() );
    screen.setEvent( &compositor );

    // Widgets.
    kvs::TransferFunctionEditor editor( &screen );
    editor.setPosition( screen.x() + screen.width(), screen.y() );
    editor.setVolumeObject( volume );
    editor.setTransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
    editor.apply(
        [&]( kvs::TransferFunction tfunc ) {
            tube_renderer->setTransferFunction( tfunc );
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

    float opa = 0.5;
    kvs::Slider opacity( &screen );
    opacity.setCaption( "Opacity" );
    opacity.setWidth( 150 );
    opacity.setMargin( 10 );
    opacity.setValue( 0.5 );
    opacity.setRange( 0, 1 );
    opacity.anchorToBottom( &checkbox );
    opacity.sliderMoved( [&] ()
    {
        opa = opacity.value();
        opacity.setCaption( "Opacity: " + kvs::String::ToString( opa ) );
    } );
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
    repetition.setValue( 20 );
    repetition.setRange( 1, 100 );
    repetition.anchorToBottom( &opacity );
    repetition.valueChanged(
        [&]() {
            compositor.setRepetitionLevel( int( repetition.value() + 0.5 ) );
            screen.redraw();
        } );
    repetition.show();

    float radius = 0.5;
    kvs::Slider radius_slider( &screen );
    radius_slider.setCaption( "Radius: " + kvs::String::ToString( radius ) );
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
        radius = kvs::Math::Clamp( v, min_value, max_value );
        radius_slider.setCaption( "Radius: " + kvs::String::From( radius ) );
    } );
    radius_slider.sliderReleased( [&] ()
    {
        compositor.setSamplingSphereRadius( radius );
        screen.redraw();
    } );

    float edge = 1.0;
    kvs::Slider edge_slider( &screen );
    edge_slider.setCaption( "Edge: " + kvs::String::ToString( edge ) );
    edge_slider.setValue( edge );
    edge_slider.setRange( 0.0, 5.0 );
    edge_slider.setMargin( 10 );
    edge_slider.anchorToBottom( &radius_slider );
    edge_slider.show();
    edge_slider.sliderMoved( [&] ()
    {
        edge = int( edge_slider.value() * 10 ) * 0.1f;
        edge_slider.setCaption( "Edge: " + kvs::String::From( edge ) );
    } );
    edge_slider.sliderReleased( [&] ()
    {
      auto* scene = screen.scene();
      auto* renderer = local::SSAOStochasticPolygonRenderer::DownCast( scene->renderer( "PolygonRenderer" ) );
      renderer->setEdgeFactor( edge );
    } );

    float edge2 = 1.0;
    kvs::Slider edge2_slider( &screen );
    edge2_slider.setCaption( "Edge2: " + kvs::String::ToString( edge2 ) );
    edge2_slider.setValue( edge2 );
    edge2_slider.setRange( 0.0, 5.0 );
    edge2_slider.setMargin( 10 );
    edge2_slider.anchorToBottom( &edge_slider );
    edge2_slider.show();
    edge2_slider.sliderMoved( [&] ()
    {
        edge2 = int( edge2_slider.value() * 10 ) * 0.1f;
        edge2_slider.setCaption( "Edge2: " + kvs::String::From( edge2 ) );
    } );
    edge2_slider.sliderReleased( [&] ()
    {
      auto* scene = screen.scene();
      auto*renderer2 = local::SSAOStochasticTubeRenderer::DownCast( scene->renderer( "StochasticTubeRenderer" ) );
      renderer2->setEdgeFactor( edge2 );
    } );

    int nsamples = 256;
    kvs::Slider nsample_slider( &screen );
    nsample_slider.setCaption( "Nsample: " + kvs::String::ToString( nsamples ) );
    nsample_slider.setValue( nsamples );
    nsample_slider.setRange( 1, 256 );
    nsample_slider.setMargin( 10 );
    nsample_slider.anchorToBottom( &edge2_slider );
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
        compositor.setNumberOfSamplingPoints( nsamples );
        screen.redraw();
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
                opacity.hide();
                repetition.hide();
            }
            else
            {
                checkbox.show();
                opacity.show();
                repetition.show();
            }
        }
        default: break;
        }
    } );
    
    kvs::ScreenCaptureEvent capture_event;
    
    screen.addEvent( &h_key );
    screen.addEvent( &capture_event );

    kvs::OrientationAxis orientation_axis( &screen, screen.scene() );
    orientation_axis.setBoxType( kvs::OrientationAxis::SolidBox );
    orientation_axis.anchorToBottomLeft();
    orientation_axis.show();

    kvs::PaintEventListener time;
    time.update( [&] ()
    {
        static size_t counter = 1;
        static float time = 0.0f;

        time += compositor.timer().msec();
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
