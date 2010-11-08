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

#include <qgraphicslinearlayout.h>

#include "EngineController.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "App.h"
#include <qlabel.h>

SpectrumAnalyzerApplet::SpectrumAnalyzerApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_visualHeight( 500 )  //Fixed value for now
    , m_running( false )
    , m_glBuffer( NULL )
    , m_settingsIcon( 0 )
    , m_detached( false )
    , m_power( true )
    , m_fullscreen( false )
    , m_cutLowFrequencys( true )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_glFormat = QGLFormat::defaultFormat();

    m_glError = true;
    
    if( !m_glFormat.hasOpenGL() )
    {
        m_glErrorText = i18n( "Your system has no OpenGL support" );
    }
    else
    {
        m_glFormat.setSampleBuffers( true );
        m_glFormat.setStencil( true );
        m_glFormat.setDoubleBuffer( true );
        m_glFormat.setAccum( true );
        m_glFormat.setDirectRendering( true );

        m_glWidget = new AnalyzerGlWidget( m_glFormat, PaletteHandler::highlightColor( 0.4, 1.05 ) );

        const QGLContext *context = m_glWidget->context();

        if( !context->isValid() )
        {
            m_glErrorText = i18n( "Could not create an OpenGL redering context." );
        }
        else if( !context->format().sampleBuffers() )
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
            m_glErrorText = "";
            m_glError = false;
            m_glBuffer = new QGLPixelBuffer( 0, 0, m_glFormat );
        }
    }

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ), this, SLOT( started() ) );
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ), this, SLOT( stopped() ) );

    connect( m_glWidget, SIGNAL( keyPressed(int) ), this, SLOT( keyPressed(int) ) );
    connect( m_glWidget, SIGNAL( hidden() ), this, SLOT( toggleDetach() ) );

    QTimer *timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( updateOpenGLScene() ) );
    timer->start( 20 );
}

SpectrumAnalyzerApplet::~SpectrumAnalyzerApplet()
{
    if ( !m_glError )
        delete( m_glWidget );
}

void
SpectrumAnalyzerApplet::keyPressed( int key )
{
    if( ( ( m_detached ) || ( m_fullscreen ) ) && ( key == Qt::Key_Escape ) )
    {
        attach();
        if ( m_detached )
        {
            detach( false );
        }
    }
}

void
SpectrumAnalyzerApplet::init()
{
    Context::Applet::init();

    resize( m_visualHeight, -1 );

    // Label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new TextScrollingWidget( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setScrollingText( i18n( "Spectrum-Analyzer" ) );
    m_headerText->setDrawBackground( true );
    m_headerText->setMinimumWidth( 170 );

    // Error Text
    m_errorText = new QGraphicsTextItem( this );
    m_errorText->setDefaultTextColor( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_errorText->setFont( labelFont );
    m_errorText->setPlainText( m_glErrorText );

    if ( !m_glError )
    {
        setCollapseHeight( m_headerText->maximumHeight() + 3.0 * standardPadding() );
        
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

        // Switch Mode Icon
        QAction* modeAction = new QAction( this );
        modeAction->setIcon( KIcon( "system-switch-user" ) );
        modeAction->setVisible( true );
        modeAction->setEnabled( true );
        modeAction->setText( i18n( "Switch Mode" ) );
        m_modeIcon = addAction( modeAction );
        connect( m_modeIcon, SIGNAL( activated() ), this, SLOT( nextMode() ) );

        KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");
        m_glWidget->setAccuracy( config.readEntry<float>( "accuracy", 1.0f ) );
        m_glWidget->setMode( config.readEntry<int>( "mode", 0 ) );
        m_glWidget->setPeaksStatus( config.readEntry<int>( "peaks", 1 ) );
        m_glWidget->setSinkrate( config.readEntry<float>( "sinkrate", 1.0f ) );
        m_glWidget->setWaveStatus( config.readEntry<int>( "wave", 0 ) );
        m_cutLowFrequencys = config.readEntry<bool>( "cutfreq", true );
        m_power = config.readEntry<bool>( "power", true );
        m_detached = config.readEntry<bool>( "detached", false );
        m_fullscreen = config.readEntry<bool>( "fullscreen", false );
    }
    else
    {
        setCollapseHeight( m_headerText->maximumHeight() + 5.0 * standardPadding() + m_errorText->boundingRect().height() );
    }

    connectSource( "spectrum-analyzer" );
    connect( dataEngine( "amarok-spectrum-analyzer" ), SIGNAL( sourceAdded( const QString & ) ), this, SLOT( connectSource( const QString & ) ) );
    
  //  dataEngine( "amarok-visualization" )->query( QString( "visualization:nbphotos:" ) + QString().setNum( m_nbPhotos ) );
  //  dataEngine( "amarok-visualization" )->query( QString( "visualization:keywords:" ) + m_KeyWords );
    setCollapseOn();
    setMinimumHeight( 0 );
    emit sizeHintChanged( Qt::MinimumSize );
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
        attach();
    }
    else
    {
        detach( false );
    }
}

void SpectrumAnalyzerApplet::togglePower()
{
    if( m_power )
    {
        m_powerIcon->action()->setIcon(  KIcon( "system-run" ) );
        m_glWidget->hide();
        m_power = false;
        setCollapseOn();
        setMinimumHeight( 0 );
        emit sizeHintChanged( Qt::MinimumSize );
    }
    else
    {
        m_powerIcon->action()->setIcon(  KIcon( "system-shutdown" ) );
        m_power = true;

        if( ( m_running ) && ( !m_glError ) )
        {
            if( !m_detached )
            {
                setCollapseOff();
                setMinimumHeight( m_visualHeight );
                emit sizeHintChanged( Qt::MinimumSize );
            }
            else
            {
                m_glWidget->show();
            }
        }
    }

    KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");
    config.writeEntry<bool>( "power", m_power );
}

void SpectrumAnalyzerApplet::nextMode()
{
    if( !m_glError )
    {
        const int mode = m_glWidget->getMode();
    
        if( mode == 0 )
        {
            m_glWidget->setMode( 1 );
        }
        else if ( mode == 1 )
        {
            m_glWidget->setMode( 2 );
        }
        else if ( mode == 2 )
        {
            m_glWidget->setMode( 3 );
        }
        else if ( mode == 3 )
        {
            m_glWidget->setMode( 0 ); //Because Channels3D are not implemented yet
        }
        else if ( mode == 4 )
        {
            m_glWidget->setMode( 0 );
        }
        else
        {
            m_glWidget->setMode( 0 );
        }

        KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");
        config.writeEntry<int>( "mode", m_glWidget->getMode() );
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
    
    qreal widmax = rect().width();

    if ( !m_glError )
    {
        widmax -= 2 * m_settingsIcon->size().width() + 2 * m_powerIcon->size().width() + 2 * m_detachIcon->size().width() + 2 * m_fullscreenIcon->size().width() + 2 * m_modeIcon->size().width() + 8 * standardPadding();
    }
    
    QRectF recto( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_headerText->setScrollingText( m_headerText->text() );
    m_headerText->setPos( ( size().width() - m_headerText->rect().width() - standardPadding() * 2.0 ) / 2 , standardPadding() + 3 );

    if ( ( !m_detached ) && ( m_power ) && ( m_running ) )
    {
        if ( m_glError )
        {
            m_errorText->setPos( ( size().width() - m_errorText->boundingRect().width() ) / 2 , standardPadding() + ( size().height() / 2 ) );
        }
        else
        {
            if( ( size().width() > 0 ) && ( size().height() > 0 ) && ( standardPadding() > 0 ) )
            {
                QGLPixelBuffer *oldBuffer = m_glBuffer;
                m_glBuffer = new QGLPixelBuffer( size().width() - 2 * standardPadding(), size().height() - recto.bottom() - 2 * standardPadding() - 20, m_glFormat );
                if ( oldBuffer )
                    delete( oldBuffer );

                m_glBuffer->makeCurrent();
                m_glWidget->initializeGLScene();
                m_glWidget->resizeGLScene( size().width() - 2 * standardPadding(), size().height() - recto.bottom() - 2 * standardPadding() - 20 );
                m_glBuffer->doneCurrent();
            }
            else
            {
                m_glError = true;
                m_glErrorText = "Could not recreate an OpenGL redering context.";
            }
        }
    }

    if ( !m_glError )
    {
        m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
        m_detachIcon->setPos( size().width() - m_detachIcon->size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
        m_fullscreenIcon->setPos( size().width() - m_fullscreenIcon->size().width() - m_detachIcon->size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
        m_modeIcon->setPos( size().width() - m_modeIcon->size().width() - m_fullscreenIcon->size().width()- m_detachIcon->size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
        m_powerIcon->setPos( size().width() - m_powerIcon->size().width() - m_modeIcon->size().width() - m_detachIcon->size().width() - m_settingsIcon->size().width() - m_fullscreenIcon->size().width() - standardPadding(), standardPadding() );
    }
}

void
SpectrumAnalyzerApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )

    bool updated = false;

    if( data.isEmpty() )
        return;

    if( ( m_running ) && ( m_power ) && ( !m_glError ) )
    {
        if ( data.contains( "message" ) && data["message"].toString().contains("clear") )
        {
            m_audioData.clear();
            updated = true;
        }

        if ( data.contains( "artist" ) )
        {
            m_artist = data[ "artist" ].toString();
            updated = true;
        }

        if ( data.contains( "title" ) )
        {
            m_title = data[ "title" ].toString();
            updated = true;
        }

        if ( data.contains( "data_left" ) )
        {
            m_audioData[ Phonon::AudioDataOutput::LeftChannel ] = data[ "data_left" ].value< QVector< qint16 > >();
            updated = true;
        }

        if ( data.contains( "data_right" ) )
        {
            m_audioData[ Phonon::AudioDataOutput::RightChannel ] = data[ "data_right" ].value< QVector< qint16 > >();
            updated = true;
        }

        if ( data.contains( "data_center" ) )
        {
            m_audioData[ Phonon::AudioDataOutput::CenterChannel ] = data[ "data_center" ].value< QVector< qint16 > >();
            updated = true;
        }

        if ( data.contains( "data_lefts" ) )
        {
            m_audioData[ Phonon::AudioDataOutput::LeftSurroundChannel ] = data[ "data_lefts" ].value< QVector< qint16 > >();
            updated = true;
        }

        if ( data.contains( "data_rights" ) )
        {
            m_audioData[ Phonon::AudioDataOutput::RightSurroundChannel ] = data[ "data_rights" ].value< QVector< qint16 > >();
            updated = true;
        }

        if ( data.contains( "data_sub" ) )
        {
            m_audioData[ Phonon::AudioDataOutput::SubwooferChannel ] = data[ "data_sub" ].value< QVector< qint16 > >();
            updated = true;
        }

        if ( updated )
            update();
    }
    else
    {
        setCollapseOn();
        setMinimumHeight( 0 );
        emit sizeHintChanged( Qt::MinimumSize );
    }
}

// SLOT
void
SpectrumAnalyzerApplet::attach()
{
    if( ( !m_glError ) && ( m_power ) && ( !m_glError ) )
    {
        //TODO: Show Pixmap
        m_glWidget->hide();
        if ( m_running )
        {
            setCollapseOff();
            setMinimumHeight( m_visualHeight );
            emit sizeHintChanged( Qt::MinimumSize );
        }
        m_detached = false;
        m_detachIcon->action()->setIcon(  KIcon( "go-up" ) );
        m_fullscreen = false;

        KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");
        config.writeEntry<bool>( "detached", m_detached );
        config.writeEntry<bool>( "fullscreen", m_fullscreen );
    }
}

void
SpectrumAnalyzerApplet::detach( bool fullscreen )
{
    if( ( !m_glError ) && ( m_power ) )
    {
        m_fullscreen = fullscreen;
        if ( m_fullscreen )
            m_glWidget->showFullScreen();
        else
            m_glWidget->show();
        m_glWidget->resize( 100, 100 );
        m_glWidget->makeCurrent();
        m_glWidget->initializeGLScene();
        m_glWidget->doneCurrent();
        setCollapseOn();
        setMinimumHeight( 0 );
        emit sizeHintChanged( Qt::MinimumSize );
        m_detached = true;
        m_detachIcon->action()->setIcon(  KIcon( "go-down" ) );

        KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");
        config.writeEntry<bool>( "detached", m_detached );
        config.writeEntry<bool>( "fullscreen", m_fullscreen );
    }
}

void
SpectrumAnalyzerApplet::transformAudioData( QVector<float> &audioData )
{
    if ( audioData.size() > 0 )
    {
        FastFourierTransformation *fft = new FastFourierTransformation( 9 );

        float *data = audioData.data();
     
        fft->spectrum( data );
        fft->scale( data, 6.0 );
     
        delete fft;
    }
}

// SLOT
void
SpectrumAnalyzerApplet::updateOpenGLScene()
{
    if ( ( !m_glError ) && ( m_running ) && ( m_power ) )
    {
        if ( m_audioData.size() > 0 )
        {
            QVector<float> iAudioData;

            for( int x = 0; x < m_audioData.values().first().size(); x++ )
            {
                float sample = 0.0f;
            
                if ( m_audioData.contains( Phonon::AudioDataOutput::LeftChannel ) )
                    sample += m_audioData[Phonon::AudioDataOutput::LeftChannel].at( x );

                if ( m_audioData.contains( Phonon::AudioDataOutput::RightChannel ) )
                    sample += m_audioData[Phonon::AudioDataOutput::RightChannel].at( x );

                if ( m_audioData.contains( Phonon::AudioDataOutput::LeftSurroundChannel ) )
                    sample += m_audioData[Phonon::AudioDataOutput::LeftSurroundChannel].at( x );

                if ( m_audioData.contains( Phonon::AudioDataOutput::RightSurroundChannel ) )
                    sample += m_audioData[Phonon::AudioDataOutput::RightSurroundChannel].at( x );

                if ( m_audioData.contains( Phonon::AudioDataOutput::SubwooferChannel ) )
                    sample += m_audioData[Phonon::AudioDataOutput::SubwooferChannel].at( x );

                iAudioData.append( sample / ( m_audioData.count() * (1<<15) ) );
            }

            transformAudioData( iAudioData );

            QVector<int> frequencyData;

            for( int y = 0; y < iAudioData.size(); y++ )
            {
                frequencyData.append( iAudioData.at(y) );
            }

            if( m_cutLowFrequencys )
                frequencyData.resize( frequencyData.size() / 2 );

            m_glWidget->setFrequencyValues( frequencyData );
        }
        
        if ( !m_detached )
        {
            m_glBuffer->makeCurrent();
            m_glWidget->paintGLScene();
            m_glBuffer->doneCurrent();
            m_glPixmap = QPixmap::fromImage( m_glBuffer->toImage() );
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
    QRect target( standardPadding(), m_headerText->maximumHeight() + 2.0 * standardPadding(), rect().width() - 2.0 * standardPadding(), rect().height() - m_headerText->maximumHeight() - 3.0 * standardPadding() );
    p->drawPixmap( target, m_glPixmap, m_glPixmap.rect() );

    // tint the whole applet
    addGradientToAppletBackground( p );
}

void
SpectrumAnalyzerApplet::started()
{
    DEBUG_BLOCK

    m_running = true;
    dataEngine( "amarok-spectrum-analyzer" )->query( QString( "data" ) );

    if( ( m_power ) && ( !m_glError ) )
    {
        if( !m_detached )
        {
            setCollapseOff();
            setMinimumHeight( m_visualHeight );
            emit sizeHintChanged( Qt::MinimumSize );
        }
        else
        {
            detach( m_fullscreen );
        }
    }
}

void
SpectrumAnalyzerApplet::stopped()
{
    DEBUG_BLOCK
    
    m_running = false;

    if( m_power )
    {
        if( m_detached )
            m_glWidget->hide();
        else
            //TODO: Hide Pixmap

        setCollapseOn();
        setMinimumHeight( 0 );
        emit sizeHintChanged( Qt::MinimumSize );
    }

    dataEngine( "amarok-spectrum-analyzer" )->query( QString( "spectrum-analyzer:stopped" ) );
}

void
SpectrumAnalyzerApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Spectrum Analyzer Settings" ), "preferences-system" );

    if( !m_glError )
    {
        ui_Settings.modeComboBox->setCurrentIndex( m_glWidget->getMode() );
        ui_Settings.interpolSpinBox->setValue( m_glWidget->getAccuracy() * 100 );
        ui_Settings.peaksCheckBox->setChecked( m_glWidget->getPeaksStatus() );
        ui_Settings.peaksSpinbox->setValue( m_glWidget->getSinkrate() * 10 );
        ui_Settings.waveCheckBox->setChecked( m_glWidget->getWaveStatus() );
        ui_Settings.cutCheckBox->setChecked( m_cutLowFrequencys );
    }
    
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}

void
SpectrumAnalyzerApplet::saveSettings()
{
    KConfigGroup config = Amarok::config("Spectrum Analyzer Applet");

    if( !m_glError )
    {
        m_glWidget->setMode( ui_Settings.modeComboBox->currentIndex() );
        m_glWidget->setAccuracy( ui_Settings.interpolSpinBox->value() / 100.0f );
        m_glWidget->setPeaksStatus( ui_Settings.peaksCheckBox->isChecked() );
        m_glWidget->setSinkrate( ui_Settings.peaksSpinbox->value() / 10.0f );
        m_glWidget->setWaveStatus( ui_Settings.waveCheckBox->isChecked() );
        m_cutLowFrequencys = ui_Settings.cutCheckBox->isChecked();

        config.writeEntry<int>( "mode", m_glWidget->getMode() );
        config.writeEntry<float>( "accuracy", m_glWidget->getAccuracy() );
        config.writeEntry<bool>( "peaks", m_glWidget->getPeaksStatus() );
        config.writeEntry<float>( "sinkrate", m_glWidget->getSinkrate() );
        config.writeEntry<bool>( "wave", m_glWidget->getWaveStatus() );
        config.writeEntry<bool>( "cutfreq", m_cutLowFrequencys );
    }
    
 /*   dataEngine( "amarok-photos" )->query( QString( "photos:nbphotos:" ) + QString().setNum( m_nbPhotos ) );
    dataEngine( "amarok-photos" )->query( QString( "photos:keywords:" ) + m_KeyWords );*/
}

#include "SpectrumAnalyzerApplet.moc"

