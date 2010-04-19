/****************************************************************************************
 * Copyright (c) 2010 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SpectrumAnalyzerApplet.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "App.h"

SpectrumAnalyzerApplet::SpectrumAnalyzerApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , EngineObserver( The::engineController() )
    , m_running( false )
    , m_glBuffer( NULL )
    , m_settingsIcon( 0 )
    , m_detached( false )
    , m_power( true )
    , m_fullscreen( false )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_glFormat = QGLFormat::defaultFormat();

    m_glError = true;
    
    if( !m_glFormat.hasOpenGL() )
    {
        m_glErrorText = i18n( "Your system has OpenGL support" );
    }
    else
    {
        m_glFormat.setSampleBuffers( true );
        m_glFormat.setStencil( true );
        m_glFormat.setDoubleBuffer( true );
        m_glFormat.setAccum( true );
        m_glFormat.setDirectRendering( true );

        QGLContext *context = new QGLContext( m_glFormat );
        /*if ( !*/context->create(); /*)
        {
            m_glErrorText = i18n( "Could not generate an OpenGL redering context." );
        }
        else*/ if( !context->format().sampleBuffers() )
        {
            m_glErrorText = i18n( "Your system has no sample buffer support." );
        }
        else if( !context->format().stencil() )
        {
            m_glErrorText = i18n( "Your system has no stencil buffer support." );
        }
        else if( !context->format().doubleBuffer() )
        {
            m_glErrorText = i18n( "Your system has no double buffer support." );
        }
        else if ( !context->format().accum() )
        {
            m_glErrorText = i18n( "Your system has no accumulation buffer support." );
        }
        else if( !context->format().directRendering() )
        {
            m_glErrorText = i18n( "Your system has no direct rendering support." );
        }
        else
        {
            m_glWidget = new AnalyzerGlWidget( context, PaletteHandler::highlightColor( 0.4, 1.05 ) );
            m_glBuffer = new QGLPixelBuffer( 0, 0, m_glFormat );
            m_glError = false;
            m_glErrorText = "";
        }
    }

     QTimer *timer = new QTimer( this );
     connect( timer, SIGNAL( timeout() ), this, SLOT( updateOpenGLScene() ) );
     timer->start( 20 );
}

SpectrumAnalyzerApplet::~SpectrumAnalyzerApplet()
{
    delete( m_glLabel );
    
    if ( !m_glError )
        delete( m_glWidget );
}

void
SpectrumAnalyzerApplet::init()
{
    m_glLabel = new QGraphicsPixmapItem( this );

    resize( 500, 300 );

    // Label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new TextScrollingWidget( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Spectrum-Analyzer" ) );

    // Error Text
    m_errorText = new QGraphicsTextItem( this );
    m_errorText->setDefaultTextColor( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_errorText->setFont( labelFont );
    m_errorText->setPlainText( m_glErrorText );

    // Set the collapse size
    setCollapseHeight( m_headerText->boundingRect().height() + 3 * standardPadding() );
    
    // Config Icon
    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( showConfigurationInterface() ) );

    // Detach Icon
    QAction* detachAction = new QAction( this );
    detachAction->setIcon( KIcon( "go-up" ) );
    detachAction->setVisible( true );
    detachAction->setEnabled( true );
    detachAction->setText( i18n( "Detach" ) );
    m_detachIcon = addAction( detachAction );
    connect( m_detachIcon, SIGNAL( activated() ), this, SLOT( toggleDetach() ) );

    // Fullscreen Icon
    QAction* fullscreenAction = new QAction( this );
    fullscreenAction->setIcon( KIcon( "transform-move" ) );
    fullscreenAction->setVisible( true );
    fullscreenAction->setEnabled( true );
    fullscreenAction->setText( i18n( "Fullscreen" ) );
    m_fullscreenIcon = addAction( fullscreenAction );
    connect( m_fullscreenIcon, SIGNAL( activated() ), this, SLOT( toggleFullscreen() ) );

    // Power Icon
    QAction* stopAction = new QAction( this );
    stopAction->setIcon( KIcon( "system-shutdown" ) );
    stopAction->setVisible( true );
    stopAction->setEnabled( true );
    stopAction->setText( i18n( "Power" ) );
    m_powerIcon = addAction( stopAction );
    connect( m_powerIcon, SIGNAL( activated() ), this, SLOT( togglePower() ) );

    KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");
/*    m_nbPhotos = config.readEntry( "NbPhotos", "10" ).toInt();
    m_Animation = config.readEntry( "Animation", "Fading" );
    m_KeyWords = config.readEntry( "KeyWords", "" );*/

    constraintsEvent();

    connectSource( "spectrum-analyzer" );
    connect( dataEngine( "amarok-spectrum-analyzer" ), SIGNAL( sourceAdded( const QString & ) ), this, SLOT( connectSource( const QString & ) ) );
    
  //  dataEngine( "amarok-visualization" )->query( QString( "visualization:nbphotos:" ) + QString().setNum( m_nbPhotos ) );
  //  dataEngine( "amarok-visualization" )->query( QString( "visualization:keywords:" ) + m_KeyWords );
    setCollapseOn();
}

void
SpectrumAnalyzerApplet::toggleFullscreen()
{
    if ( !m_fullscreen )
    {
        if ( !m_detached )
        {
            detach( true );
        }
        else
        {
            attach();
            detach( true );
        }
    }
    else
    {
        attach();
        if ( m_detached )
        {
            detach( false );
        }
    }
}

void
SpectrumAnalyzerApplet::toggleDetach()
{
    if( m_detached )
    {
        m_detachIcon->action()->setIcon(  KIcon( "go-up" ) );
        attach();
    }
    else
    {
        m_detachIcon->action()->setIcon(  KIcon( "go-down" ) );
        detach( false );
    }
}

void SpectrumAnalyzerApplet::togglePower()
{
    if( m_power )
    {
        m_powerIcon->action()->setIcon(  KIcon( "system-run" ) );
        m_glWidget->hide();
        m_glLabel->hide();
        m_power = false;
        setCollapseOn();
    }
    else
    {
        m_powerIcon->action()->setIcon(  KIcon( "system-shutdown" ) );
        m_power = true;
        if( ( m_running ) && ( !m_detached ) )
        {
            m_glLabel->show();
            setCollapseOff();
        }
        else if ( m_running )
        {
            m_glWidget->show();
        }
    }
}

void
SpectrumAnalyzerApplet::connectSource( const QString &source )
{
    if ( source == "spectrum-analyzer" )
        dataEngine( "amarok-spectrum-analyzer" )->connectSource( "spectrum-analyzer", this );
}

void
SpectrumAnalyzerApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    
    qreal widmax = boundingRect().width() - 2 * m_settingsIcon->size().width() - 2 * m_powerIcon->size().width() - 2 * m_detachIcon->size().width() - 2 * m_fullscreenIcon->size().width() - 6 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_headerText->setScrollingText( m_headerText->text(), rect );
    m_headerText->setPos( ( size().width() - m_headerText->boundingRect().width() ) / 2 , standardPadding() + 3 );

    if ( m_glError )
    {
        m_errorText->setPos( ( size().width() - m_errorText->boundingRect().width() ) / 2 , standardPadding() + ( size().height() / 2 ) );
    }
    else if ( !m_detached )
    {
        QGLPixelBuffer *oldBuffer = m_glBuffer;
        m_glBuffer = new QGLPixelBuffer( size().width() - 2 * standardPadding(), size().height() - rect.bottom() - 2 * standardPadding() - 20, m_glFormat );
        if ( oldBuffer )
            delete( oldBuffer );

        m_glBuffer->makeCurrent();
        m_glWidget->initializeGLScene();
        m_glWidget->resizeGLScene( size().width() - 2 * standardPadding(), size().height() - rect.bottom() - 2 * standardPadding() - 20 );
        m_glBuffer->doneCurrent();
        m_glLabel->setPos( standardPadding(), 20 + rect.bottom() + standardPadding() );
    }

    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
    m_detachIcon->setPos( size().width() - m_detachIcon->size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
    m_fullscreenIcon->setPos( size().width() - m_fullscreenIcon->size().width() - m_detachIcon->size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
    m_powerIcon->setPos( size().width() - m_powerIcon->size().width() - m_detachIcon->size().width() - m_settingsIcon->size().width() - m_fullscreenIcon->size().width() - standardPadding(), standardPadding() );
}

void
SpectrumAnalyzerApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK

    Q_UNUSED( name )

    if( data.isEmpty() )
        return;

    if(  ( m_running ) && ( m_power ) )
    {
        if ( data.contains( "message" ) && data["message"].toString().contains("clear") )
        {
            m_audioData.clear();
            if ( !m_detached )
                m_glLabel->hide();
            else
                m_glWidget->hide();
            setCollapseOn();
        }
        else if ( data.contains( "artist" ) )
        {
            m_artist = data[ "artist" ].toString();
        }
        else if ( data.contains( "title" ) )
        {
            m_title = data[ "title" ].toString();
        }
        else
        {
            if ( data.contains( "data_left" ) )
            {
                m_audioData[ Phonon::AudioDataOutput::LeftChannel ] = data[ "data_left" ].value< QVector< qint16 > >();
            }
            else if ( data.contains( "data_right" ) )
            {
                m_audioData[ Phonon::AudioDataOutput::RightChannel ] = data[ "data_right" ].value< QVector< qint16 > >();
            }
            else if ( data.contains( "data_center" ) )
            {
                m_audioData[ Phonon::AudioDataOutput::CenterChannel ] = data[ "data_center" ].value< QVector< qint16 > >();
            }
            else if ( data.contains( "data_lefts" ) )
            {
                m_audioData[ Phonon::AudioDataOutput::LeftSurroundChannel ] = data[ "data_lefts" ].value< QVector< qint16 > >();
            }
            else if ( data.contains( "data_rights" ) )
            {
                m_audioData[ Phonon::AudioDataOutput::RightSurroundChannel ] = data[ "data_rights" ].value< QVector< qint16 > >();
            }
            else if ( data.contains( "data_sub" ) )
            {
                m_audioData[ Phonon::AudioDataOutput::SubwooferChannel ] = data[ "data_sub" ].value< QVector< qint16 > >();

            }

            qDebug() << "data data data";

            if ( !m_detached )
                setCollapseOff();
        }
    }
    else
    {
        setCollapseOn();
    }

    update();
    updateConstraints();
}

// SLOT
void
SpectrumAnalyzerApplet::attach()
{
    if( ( !m_glError ) && ( m_power ) )
    {
        m_glLabel->show();
        m_glWidget->hide();
        if ( m_running )
            setCollapseOff();
        m_detached = false;
    }
}

void
SpectrumAnalyzerApplet::detach( bool fullscreen )
{
    if( ( !m_glError ) && ( m_power ) )
    {
        m_fullscreen = fullscreen;
        m_glLabel->hide();
        if ( m_fullscreen )
            m_glWidget->showFullScreen();
        else
            m_glWidget->show();
        m_glWidget->resize( 100, 100 );
        m_glWidget->makeCurrent();
        m_glWidget->initializeGLScene();
        m_glWidget->doneCurrent();
        setCollapseOn();
        m_detached = true;
    }
}

// SLOT
void
SpectrumAnalyzerApplet::updateOpenGLScene()
{
    if ( ( !m_glError ) && ( m_running ) && ( m_power ) )
    {
        //TODO: Do Fast Fourier Transform and send data to widget
        //m_glWidget->setFrequencyValues();
        
        if ( !m_detached )
        {
            m_glBuffer->makeCurrent();
            m_glWidget->paintGLScene();
            m_glBuffer->doneCurrent();
            m_glLabel->setPixmap( QPixmap::fromImage( m_glBuffer->toImage() ) );
        }
        else
        {
            m_glWidget->updateGL();
        }
    }
}


void
SpectrumAnalyzerApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );

    p->setRenderHint( QPainter::Antialiasing );
    
    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title (only if not animating )
    if ( !m_headerText->isAnimating() )
        drawRoundedRectAroundText( p, m_headerText );
}

void
SpectrumAnalyzerApplet::engineNewTrackPlaying( )
{
    m_running = true;
    dataEngine( "amarok-spectrum-analyzer" )->query( QString( "data" ) );
    if( ( !m_detached ) && ( m_power ) )
    {
        setCollapseOff();
        m_glLabel->show();
    }
    else if ( m_power )
    {
        m_glWidget->show();
    }
    
}

void
SpectrumAnalyzerApplet::enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason )
{
    Q_UNUSED( finalPosition )
    Q_UNUSED( trackLength )

    if( m_power )
    {
        if( !m_detached )
            m_glLabel->hide();
        else
            m_glWidget->hide();
        m_running = false;
        setCollapseOn();
    }
    dataEngine( "amarok-spectrum-analyzer" )->query( QString( "spectrum-analyzer:stopped" ) );
}

void
SpectrumAnalyzerApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
/*    ui_Settings.setupUi( settings );*/

    parent->addPage( settings, i18n( "Spectrum Analyzer Settings" ), "preferences-system" );

    /*ui_Settings.animationComboBox->setCurrentIndex( ui_Settings.animationComboBox->findText( m_Animation ) );
    ui_Settings.photosSpinBox->setValue( m_nbPhotos );
    ui_Settings.additionalkeywordsLineEdit->setText( m_KeyWords );*/
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}

void
SpectrumAnalyzerApplet::saveSettings()
{
    KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");

/*    m_nbPhotos = ui_Settings.photosSpinBox->value();
    m_Animation = ui_Settings.animationComboBox->currentText();
    m_KeyWords = ui_Settings.additionalkeywordsLineEdit->text();
    config.writeEntry( "NbPhotos", m_nbPhotos );
    config.writeEntry( "Animation", m_Animation );
    config.writeEntry( "KeyWords", m_KeyWords );

    m_widget->setMode( ui_Settings.animationComboBox->currentIndex() );

    m_widget->clear();

    dataEngine( "amarok-photos" )->query( QString( "photos:nbphotos:" ) + QString().setNum( m_nbPhotos ) );
    dataEngine( "amarok-photos" )->query( QString( "photos:keywords:" ) + m_KeyWords );*/
}

#include "SpectrumAnalyzerApplet.moc"

