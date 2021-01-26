#include "Controller.h"
#include "Model.h"
#include "View.h"


namespace local
{

Controller::Controller( local::Model& model, local::View& view ):
    m_model( model ),
    m_view( view ),
    m_ssao_check_box( &m_view.screen() ),
    m_lod_check_box( &m_view.screen() ),
    m_repeat_slider( &m_view.screen() ),
    m_radius_slider( &m_view.screen() ),
    m_points_slider( &m_view.screen() ),
    m_edge_slider( &m_view.screen() ),
    m_axis( &m_view.screen(), m_view.screen().scene() ),
    m_bar( &m_view.screen() ),
    m_editor( &m_view.screen() )
{
    m_ssao_check_box.setCaption( "SSAO" );
    m_ssao_check_box.setState( m_model.isSSAOEnabled() );
    m_ssao_check_box.setMargin( 10 );
    m_ssao_check_box.stateChanged( [&] ()
    {
        m_model.setSSAOEnabled( m_ssao_check_box.state() );
        m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
    } );

    m_lod_check_box.setCaption( "LOD" );
    m_lod_check_box.setState( m_model.isLODEnabled() );
    m_lod_check_box.setMargin( 10 );
    m_lod_check_box.anchorToBottom( &m_ssao_check_box );
    m_lod_check_box.stateChanged( [&] ()
    {
        m_model.setLODEnabled( m_lod_check_box.state() );
        auto* scene = m_view.screen().scene();
        if ( m_model.isSSAOEnabled() )
        {
            auto* renderer = local::Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( m_model.isLODEnabled() );
        }
        else
        {
            auto* renderer = local::Model::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( m_model.isLODEnabled() );
        }
    } );

    m_repeat_slider.setCaption( "Repeats: " + kvs::String::ToString( m_model.repeats() ) );
    m_repeat_slider.setValue( m_model.repeats() );
    m_repeat_slider.setRange( 1, 100 );
    m_repeat_slider.setMargin( 10 );
    m_repeat_slider.anchorToBottom( &m_lod_check_box );
    m_repeat_slider.sliderMoved( [&] ()
    {
        m_model.setRepeats( m_repeat_slider.value() );
        m_repeat_slider.setCaption( "Repeats: " + kvs::String::From( m_model.repeats() ) );
    } );
    m_repeat_slider.sliderReleased( [&] ()
    {
        auto* scene = m_view.screen().scene();
        if ( m_model.isSSAOEnabled() )
        {
            auto* renderer = local::Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( m_model.repeats() );
        }
        else
        {
            auto* renderer = local::Model::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( m_model.repeats() );
        }
    } );

    m_radius_slider.setCaption( "Radius: " + kvs::String::ToString( m_model.radius() ) );
    m_radius_slider.setValue( m_model.radius() );
    m_radius_slider.setRange( 0.1, 5.0 );
    m_radius_slider.setMargin( 10 );
    m_radius_slider.anchorToBottom( &m_repeat_slider );
    m_radius_slider.sliderMoved( [&] ()
    {
        const float min_value = m_radius_slider.minValue();
        const float max_value = m_radius_slider.maxValue();
        const float v = int( m_radius_slider.value() * 2 ) * 0.5f;
        m_model.setRadius( kvs::Math::Clamp( v, min_value, max_value ) );
        m_radius_slider.setCaption( "Radius: " + kvs::String::From( m_model.radius() ) );
    } );
    m_radius_slider.sliderReleased( [&] ()
    {
        if ( m_model.isSSAOEnabled() )
        {
            m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
        }
    } );

    m_points_slider.setCaption( "Points: " + kvs::String::From( m_model.points() ) );
    m_points_slider.setValue( m_model.points() );
    m_points_slider.setRange( 1, 256 );
    m_points_slider.setMargin( 10 );
    m_points_slider.anchorToBottom( &m_radius_slider );
    m_points_slider.sliderMoved( [&] ()
    {
        m_model.setPoints( int( m_points_slider.value() ) );
        m_points_slider.setCaption( "Points: " + kvs::String::From( m_model.points() ) );
    } );
    m_points_slider.sliderReleased( [&] ()
    {
        if ( m_model.isSSAOEnabled() )
        {
            m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
        }
    } );

    m_edge_slider.setCaption( "Edge: " + kvs::String::From( m_model.edge() ) );
    m_edge_slider.setValue( m_model.edge() );
    m_edge_slider.setRange( 0.1, 5 );
    m_edge_slider.setMargin( 10 );
    m_edge_slider.anchorToBottom( &m_points_slider );
    m_edge_slider.sliderMoved( [&] ()
    {
        const float v = int( m_edge_slider.value() * 10 ) * 0.1f;
        m_model.setEdgeFactor( v );
        m_edge_slider.setCaption( "Edge: " + kvs::String::From( m_model.edge() ) );
    } );
    m_edge_slider.sliderReleased( [&] ()
    {
        if ( m_model.isSSAOEnabled() )
        {
            m_view.screen().scene()->replaceRenderer( "Renderer", m_model.renderer() );
        }
    });

    m_axis.setBoxType( kvs::OrientationAxis::SolidBox );
    m_axis.anchorToBottomLeft();
    m_axis.show();

    m_bar.setCaption( "Velocity Magnitude" );
    m_bar.setColorMap( m_model.transferFunction().colorMap() );
    m_bar.anchorToBottomRight();

    m_editor.setTransferFunction( m_model.transferFunction() );
    m_editor.show();
    m_editor.apply( [&] ( kvs::TransferFunction tfunc )
    {
        m_model.setTransferFunction( tfunc );
        auto* scene = m_view.screen().scene();
        if ( m_model.isSSAOEnabled() )
        {
            auto* renderer = local::Model::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( tfunc );
        }
        else
        {
            auto* renderer = local::Model::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( tfunc );
        }
        m_bar.setColorMap( tfunc.colorMap() );
        m_view.screen().redraw();
    } );

    m_key_press_event.update( [&] ( kvs::KeyEvent* event )
    {
        switch ( event->key() )
        {
        case kvs::Key::i:
        {
            if ( this->isVisible() ) { this->hide(); }
            else { this->show(); }
            m_view.screen().redraw();
            break;
        }
        default: break;
        }
    } );
    m_view.screen().addEvent( &m_paint_event );
    m_view.screen().addEvent( &m_key_press_event );
    m_view.screen().addEvent( &m_capture_event );
    m_view.screen().addEvent( &m_target_change_event );

    this->show();
}

bool Controller::isVisible() const
{
    return m_ssao_check_box.isVisible();
}

void Controller::show()
{
    m_ssao_check_box.show();
    m_lod_check_box.show();
    m_repeat_slider.show();
    m_radius_slider.show();
    m_points_slider.show();
    m_edge_slider.show();
    m_axis.show();
    m_bar.show();
}

void Controller::hide()
{
    m_ssao_check_box.hide();
    m_lod_check_box.hide();
    m_repeat_slider.hide();
    m_radius_slider.hide();
    m_points_slider.hide();
    m_edge_slider.hide();
    m_axis.hide();
    m_bar.hide();
}

} // end of namespace local
