#include "Widget.h"
#include "Vis.h"
#include <kvs/CheckBox>
#include <kvs/CheckBoxGroup>
#include <kvs/Slider>


namespace
{

class SSAOCheckBox : public kvs::CheckBox
{
private:
    kvs::Scene* m_scene;
    local::Input& m_input;
    local::Vis& m_vis;

public:
    SSAOCheckBox( kvs::glut::Screen& screen, local::Input& input, local::Vis& vis ):
        kvs::CheckBox( &screen ),
        m_scene( screen.scene() ),
        m_input( input ),
        m_vis( vis )
    {
        setCaption( "SSAO" );
        setState( input.ssao );
        setMargin( 10 );
    }

    void stateChanged()
    {
        if ( state() )
        {
            const local::Vis::NoSSAORenderer* renderer = local::Vis::NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.ssao = true;
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
        }
        else
        {
            const local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.ssao = false;
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
            m_input.shader = renderer->ShaderMode();
        }
        m_scene->replaceRenderer( "Renderer", m_vis.renderer() );
    }
};

class EECheckBox : public kvs::CheckBox
{
private:
    kvs::Scene* m_scene;
    local::Input& m_input;
    local::Vis& m_vis;

public:
    EECheckBox( kvs::glut::Screen& screen, local::Input& input, local::Vis& vis ):
        kvs::CheckBox( &screen ),
        m_scene( screen.scene() ),
        m_input( input ),
        m_vis( vis )
    {
        setCaption( "Edge" );
        setState( input.ee );
        setMargin( 10 );
    }

    void stateChanged()
    {
        const bool ssao = ( local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            if ( state() )
            {
                local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
                m_input.repeats = renderer->repetitionLevel();
                m_input.tfunc = renderer->transferFunction();
                m_input.ee = true;
                m_input.shader = 1;
            }
            else
            {
                local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
                m_input.repeats = renderer->repetitionLevel();
                m_input.tfunc = renderer->transferFunction();
                m_input.ee = false;
                m_input.shader = 0;
            }
            m_scene->replaceRenderer( "Renderer", m_vis.renderer() );
        }
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
        const bool ssao = ( local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setEnabledLODControl( state() );
        }
        else
        {
            local::Vis::NoSSAORenderer* renderer = local::Vis::NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
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
        const bool ssao = ( local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( size_t( value() ) );
        }
        else
        {
            local::Vis::NoSSAORenderer* renderer = local::Vis::NoSSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            renderer->setRepetitionLevel( size_t( value() ) );
        }
    }
};

class SamplingRadiusSlider : public kvs::Slider
{
private:
    kvs::Scene* m_scene;
    local::Input& m_input;
    local::Vis& m_vis;

public:
    SamplingRadiusSlider( kvs::glut::Screen& screen, local::Input& input, local::Vis& vis ):
        kvs::Slider( &screen ),
        m_scene( screen.scene() ),
        m_input( input ),
        m_vis( vis )
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
        const bool ssao = ( local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            const local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
            m_scene->replaceRenderer( "Renderer", m_vis.renderer() );
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
    local::Vis& m_vis;

public:
    SamplingPointsSlider( kvs::glut::Screen& screen, local::Input& input, local::Vis& vis ):
        kvs::Slider( &screen ),
        m_scene( screen.scene() ),
        m_input( input ),
        m_vis( vis )
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
        const bool ssao = ( local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( m_scene->renderer( "Renderer" ) );
            m_input.repeats = renderer->repetitionLevel();
            m_input.tfunc = renderer->transferFunction();
            m_scene->replaceRenderer( "Renderer", m_vis.renderer() );
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
    kvs::ColorMapBar* m_cmap_bar;

public:
    TransferFunctionEditor( kvs::glut::Screen& screen, kvs::ColorMapBar* cmap_bar ):
        kvs::glut::TransferFunctionEditor( &screen ),
        m_cmap_bar( cmap_bar ) {}

    void apply()
    {
        kvs::Scene* scene = static_cast<kvs::glut::Screen*>( screen() )->scene();
        const bool ssao = ( local::Vis::SSAORenderer::DownCast( scene->renderer( "Renderer" ) ) != NULL );
        if ( ssao )
        {
            local::Vis::SSAORenderer* renderer = local::Vis::SSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( transferFunction() );
        }
        else
        {
            local::Vis::NoSSAORenderer* renderer = local::Vis::NoSSAORenderer::DownCast( scene->renderer( "Renderer" ) );
            renderer->setTransferFunction( transferFunction() );
        }
        m_cmap_bar->setColorMap( colorMap() );
        screen()->redraw();
    }
};

} // end of namespace


namespace local
{

Widget::Widget( kvs::glut::Screen& screen, local::Input& input, local::Vis& vis )
{
    SSAOCheckBox* ssao_check_box = new SSAOCheckBox( screen, input, vis );
    ssao_check_box->show();
    m_ui_widgets.push_back( ssao_check_box );

    kvs::WidgetBase* parent = ssao_check_box;
    EECheckBox* ee_check_box = new EECheckBox( screen, input, vis );
    ee_check_box->setPosition( parent->x(), parent->y() + parent->height() - 10 );
    ee_check_box->show();
    m_ui_widgets.push_back( ee_check_box );

    parent = ee_check_box;
    LODCheckBox* lod_check_box = new LODCheckBox( screen, input );
    lod_check_box->setPosition( parent->x(), parent->y() + parent->height() - 10 );
    lod_check_box->show();
    m_ui_widgets.push_back( lod_check_box );

    parent = lod_check_box;
    RepeatSlider* repeat_slider = new RepeatSlider( screen, input );
    repeat_slider->setPosition( parent->x(), parent->y() + parent->height() );
    repeat_slider->show();
    m_ui_widgets.push_back( repeat_slider );

    parent = repeat_slider;
    SamplingRadiusSlider* radius_slider = new SamplingRadiusSlider( screen, input, vis );
    radius_slider->setPosition( parent->x(), parent->y() + parent->height() - 10 );
    radius_slider->show();
    m_ui_widgets.push_back( radius_slider );

    parent = radius_slider;
    SamplingPointsSlider* points_slider = new SamplingPointsSlider( screen, input, vis );
    points_slider->setPosition( parent->x(), parent->y() + parent->height() - 10 );
    points_slider->show();
    m_ui_widgets.push_back( points_slider );

    m_axis = new OrientationAxis( screen );
    m_axis->setBoxType( kvs::OrientationAxis::SolidBox );
    m_axis->show();

    m_bar = new ColorMapBar( screen );
    m_bar->setCaption( "Velocity Magnitude" );
    m_bar->setColorMap( input.tfunc.colorMap() );
    m_bar->show();

    m_editor = new TransferFunctionEditor( screen, m_bar );
    m_editor->setTransferFunction( input.tfunc );
    m_editor->show();
}

Widget::~Widget()
{
    for ( size_t i = 0; i < m_ui_widgets.size(); i++ ) { if ( m_ui_widgets[i] ) { delete m_ui_widgets[i]; } }
    if ( m_axis ) { delete m_axis; }
    if ( m_bar ) { delete m_bar; }
    if ( m_editor ) { delete m_editor; }
}

bool Widget::isShown() const
{
    return ( m_ui_widgets.size() > 0 && m_ui_widgets[0] != NULL ) ? m_ui_widgets[0]->isShown() : false;
}

void Widget::show()
{
    for ( size_t i = 0; i < m_ui_widgets.size(); i++ ) { m_ui_widgets[i]->show(); }
    m_axis->show();
    m_bar->show();
}

void Widget::hide()
{
    for ( size_t i = 0; i < m_ui_widgets.size(); i++ ) { m_ui_widgets[i]->hide(); }
    m_axis->hide();
    m_bar->hide();
}

} // end of namespace local
