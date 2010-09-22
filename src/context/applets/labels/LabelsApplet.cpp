/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#include "LabelsApplet.h"
#include "LabelGraphicsItem.h"

// Amarok
#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/meta/Meta.h"
#include "Theme.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/capabilities/UpdateCapability.h"
#include "amarokurls/AmarokUrl.h"

// KDE
#include <KConfigDialog>
#include <KGlobalSettings>
#include <KStandardDirs>
#include <KMessageBox>
#include <KComboBox>
#include <Plasma/IconWidget>
#include <Plasma/Containment>

// Qt
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QGraphicsScene>


LabelsApplet::LabelsApplet( QObject *parent, const QVariantList &args )
    : Context::Applet( parent, args )
    , Engine::EngineObserver( The::engineController() )
    , m_titleLabel( 0 )
    , m_addLabel( 0 )
    , m_addLabelProxy( 0 )
    , m_reloadIcon( 0 )
    , m_settingsIcon( 0 )
{
    setHasConfigurationInterface( true );
}

LabelsApplet::~LabelsApplet()
{
    qDeleteAll( m_labelItems );
    delete m_addLabelProxy;

    delete m_reloadIcon;
    delete m_settingsIcon;
}

void
LabelsApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    setBackgroundHints( Plasma::Applet::NoBackground );

    // properly set the size, asking for the whole cv size.
    resize( 500, -1 );

    // this applet has to be on top of the applet below, otherwise the completion list of the combobox will shine through the other applet
    setZValue( zValue() + 100 );

    // Create the title label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_titleLabel = new TextScrollingWidget( this );
    m_titleLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_titleLabel->setFont( labelFont );
    m_titleText = i18n( "Labels" );

    // Set the collapse size
    setCollapseHeight( m_titleLabel->boundingRect().height() + 3 * standardPadding() );

    // reload icon
    QAction *reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload" ) );
    m_reloadIcon = addAction( reloadAction );
    m_reloadIcon->setEnabled( false );
    connect( m_reloadIcon, SIGNAL( clicked() ), this, SLOT( reload() ) );

    // settings icon
    QAction *settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( showConfigurationInterface() ) );

    m_addLabelProxy = new QGraphicsProxyWidget( this );
    m_addLabelProxy->setAttribute( Qt::WA_NoSystemBackground );
    m_addLabel = new KComboBox( this );
    m_addLabel->setAttribute( Qt::WA_NoSystemBackground );
    m_addLabel->setAutoFillBackground( false );
    m_addLabel->completionObject()->setIgnoreCase( true );
    m_addLabel->setCompletionMode( KGlobalSettings::CompletionPopup );
    connect( m_addLabel, SIGNAL( returnPressed() ), this, SLOT( addLabelPressed() ) );
    m_addLabelProxy->setWidget( m_addLabel );
    m_addLabelProxy->hide();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Labels Applet");
    m_minCount = config.readEntry( "MinCount", "30" ).toInt();
    m_numLabels = config.readEntry( "NumLabels", "10" ).toInt();
    m_personalCount = config.readEntry( "PersonalCount", "60" ).toInt();
    m_autoAdd = (bool)config.readEntry( "AutoAdd", "0" ).toInt();
    m_minAutoAddCount = config.readEntry( "MinAutoAddCount", "60" ).toInt();
    QStringList defaultBlacklist;
    defaultBlacklist << "favourite" << "like it";
    m_blacklist = config.readEntry( "Blacklist", defaultBlacklist );

    m_stoppedstate = true;
    setCollapseOn();

    constraintsEvent();

    connectSource( "labels" );
    connect( dataEngine( "amarok-labels" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    startDataQuery();
}

void
LabelsApplet::startDataQuery()
{
    Collections::QueryMaker *label = CollectionManager::instance()->queryMaker()->setQueryType( Collections::QueryMaker::Label );

    QList<Collections::QueryMaker*> queries;
    queries << label;

    //MetaQueryMaker will run multiple different queries just fine as long as we do not use it
    //to set the query type. Configuring the queries is ok though

    Collections::QueryMaker *mqm = new Collections::MetaQueryMaker( queries );
    connect( mqm, SIGNAL( newResultReady( QString, Meta::LabelList ) ), SLOT( resultReady( QString, Meta::LabelList ) ), Qt::DirectConnection );

    mqm->setAutoDelete( true );

    mqm->run();
}

void
LabelsApplet::resultReady( const QString &collectionId, const Meta::LabelList &labels )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::LabelPtr &label, labels )
    {
        if( !label->name().isEmpty() )
            m_allLabels << label->name();
    }

    QString saveText = m_addLabel->lineEdit()->text();
    m_allLabels.sort();
    m_addLabel->clear();
    m_addLabel->insertItems( 0, m_allLabels );
    m_addLabel->completionObject()->setItems( m_allLabels );
    m_addLabel->lineEdit()->setText( saveText );
}

void
LabelsApplet::engineNewTrackPlaying( )
{
    if( !m_addLabelProxy )
        return;

    DEBUG_BLOCK
    m_currentLabels.clear();
    m_labelInfos.clear();

    m_addLabelProxy->show();
    m_addLabel->clearEditText();
    m_reloadIcon->setEnabled( true );

    m_stoppedstate = false;
    m_titleText = i18n( "Labels" );

    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track )
    {
        foreach( const Meta::LabelPtr &label, track->labels() )
        {
            m_currentLabels += label->name();
        }
    }

    updateLabels();
    
    dataEngine( "amarok-labels" )->query( QString( "labels" ) );
}

void
LabelsApplet::reload()
{
    if( !m_stoppedstate )
    {
        m_labelInfos.clear();
        dataEngine( "amarok-labels" )->query( QString( "labels:reload" ) );
    }
}

void
LabelsApplet::enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason )
{
    Q_UNUSED( finalPosition )
    Q_UNUSED( trackLength )
    DEBUG_BLOCK

    if( !m_addLabelProxy )
        return;

    m_currentLabels.clear();
    m_labelInfos.clear();
    updateLabels();

    m_stoppedstate = true;
    m_titleText = i18n( "Labels" ) + QString( " : " ) + i18n( "No track playing" );
    m_addLabelProxy->hide();
    m_reloadIcon->setEnabled( false );
    setBusy( false );
    setMinimumHeight( 0 );
    setCollapseOn();
    dataEngine( "amarok-labels" )->query( QString( "labels:stopped" ) );
}

void
LabelsApplet::updateLabels()
{
    qDeleteAll( m_labelItems );
    m_labelItems.clear();
    
    QMap < QString, int > tempLabelsMap;
    QMap < QString, int > finalLabelsMap;

    // add the user assigned labels directly to the final map
    for( int i = 0; i < m_currentLabels.count(); i++ )
    {
        if( !m_blacklist.contains( m_currentLabels.at( i ) ) )
        {
            finalLabelsMap.insert( m_currentLabels.at( i ), m_personalCount );
        }
    }
    // add the downloaded labels to the temp map first (if they aren't alreday in the final map / update value in final map if necessary)
    QMapIterator < QString, QVariant > it_infos ( m_labelInfos );
    while( it_infos.hasNext() )
    {
        it_infos.next();
        if( !finalLabelsMap.contains( it_infos.key() ) && !m_blacklist.contains( it_infos.key() ) && it_infos.value().toInt() >= m_minCount )
        {
            tempLabelsMap.insert( it_infos.key(), it_infos.value().toInt() );
        }
        else if( finalLabelsMap.contains( it_infos.key() ) )
        {
            finalLabelsMap[ it_infos.key() ] = it_infos.value().toInt();
        }
    }
    // then sort the values of the temp map
    QList < int > tempLabelsValues = tempLabelsMap.values();
    qSort( tempLabelsValues.begin(), tempLabelsValues.end(), qGreater < int > () );
    // and copy the highest rated labels to the final map until max. number is reached
    const int additionalNum = m_numLabels - finalLabelsMap.count();
    if( additionalNum > 0 && tempLabelsValues.count() > 0 )
    {
        int minCount;
        if( additionalNum <= tempLabelsValues.count() )
            minCount = tempLabelsValues.at( additionalNum - 1 );
        else
            minCount = tempLabelsValues.last();
        QMapIterator < QString, int > it_temp ( tempLabelsMap );
        while( it_temp.hasNext() )
        {
            it_temp.next();
            if( it_temp.value() >= minCount )
            {
                finalLabelsMap.insert( it_temp.key(), it_temp.value() );
                
                if( finalLabelsMap.count() >= m_numLabels )
                    break;
            }
        }
    }
    // and finally create the LabelGraphicsItems
    QMapIterator < QString, int > it_final ( finalLabelsMap );
    while( it_final.hasNext() )
    {
        it_final.next();
        
        if( it_final.key().isEmpty() ) // empty labels don't make sense but they cause a freeze
            continue;

        int i_size = (int)( it_final.value() / 10 - 5 );
        if( i_size < -1 )
            i_size = -1;

        LabelGraphicsItem *labelGraphics = new LabelGraphicsItem( it_final.key(), i_size, this );
        if( m_currentLabels.contains( it_final.key() ) )
            labelGraphics->setSelected( true );
        connect( labelGraphics, SIGNAL( toggled( const QString & ) ), this, SLOT( toggleLabel( const QString & ) ) );
        connect( labelGraphics, SIGNAL( list( const QString & ) ), this, SLOT( listLabel( const QString & ) ) );
        connect( labelGraphics, SIGNAL( blacklisted( const QString & ) ), this, SLOT( blacklistLabel( const QString & ) ) );
        m_labelItems.append( labelGraphics );
    }

    constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
    update();
}

void
LabelsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();

    qreal widmax = boundingRect().width() - 2 * m_reloadIcon->size().width() - 2 * m_settingsIcon->size().width() - 8 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_titleLabel->setScrollingText( m_titleText, rect );
    m_titleLabel->setPos( ( size().width() - m_titleLabel->boundingRect().width() ) / 2 , standardPadding() + 3 );

    m_reloadIcon->setPos( size().width() - m_reloadIcon->size().width() - m_settingsIcon->size().width() - 2 * standardPadding(), standardPadding() );
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );

    if( !m_stoppedstate )
    {
        qreal x_pos;
        qreal y_pos = m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding();
        qreal width = 0;
        qreal height = 0;
        int start_index = 0;
        int end_index = -1;
        qreal max_width = size().width() - 2 * standardPadding();
        for( int i=0; i<m_labelItems.count(); i++ )
        {
            QRectF l_size = m_labelItems.at(i)->boundingRect();
            if( width + l_size.width() + 3 * standardPadding() <= max_width || i == 0 )
            {
                width += l_size.width();
                if( i != 0 )
                    width += standardPadding();
                if( l_size.height() > height )
                    height = l_size.height();
                end_index = i;
            }
            else
            {
                x_pos = ( max_width - width ) / 2;
                for( int j=start_index; j<=end_index; j++ )
                {
                    QRectF c_size = m_labelItems.at(j)->boundingRect();
                    m_labelItems[j]->setPos( x_pos, y_pos + (height-c_size.height())/2 );
                    x_pos += c_size.width() + standardPadding();
                }
                y_pos += height; // no padding needed
                width = l_size.width();
                height = l_size.height();
                start_index = i;
                end_index = i;
            }
        }
        x_pos = ( max_width - width ) / 2;
        for( int j = start_index; j <= end_index; j++ )
        {
            QRectF c_size = m_labelItems.at(j)->boundingRect();
            m_labelItems[j]->setPos( x_pos, y_pos + (height-c_size.height())/2 );
            x_pos += c_size.width() + standardPadding();
        }
        if( m_labelItems.count() > 0 )
            y_pos += height + standardPadding();

        qreal addLabelProxyWidth = size().width() - 2 * standardPadding();
        if( addLabelProxyWidth > 300 )
            addLabelProxyWidth = 300;
        m_addLabelProxy->setPos( ( size().width() - addLabelProxyWidth ) / 2, y_pos );
        m_addLabelProxy->setMinimumWidth( addLabelProxyWidth );
        m_addLabelProxy->setMaximumWidth( addLabelProxyWidth );
        y_pos += m_addLabelProxy->size().height() + standardPadding();

        resize( size().width(), y_pos );
        setMinimumHeight( y_pos );
        setMaximumHeight( y_pos );
        emit sizeHintChanged( Qt::PreferredSize );
    }
}

void
LabelsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );
    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title (only if not animating )
    if ( !m_titleLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_titleLabel );
}

void
LabelsApplet::connectSource( const QString &source )
{
    if ( source == "labels" )
        dataEngine( "amarok-labels" )->connectSource( "labels", this );
}

void
LabelsApplet::dataUpdated( const QString &name, const Plasma::DataEngine::Data &data ) // SLOT
{
    Q_UNUSED( name )

    if ( data.empty() )
        return;

    // if we get a message, show it
    if ( data.contains( "message" ) && data["message"].toString().contains("Fetching") )
    {
        m_titleText = i18n( "Labels" ) + QString( " : " ) + i18n( "Fetching ..." );
        constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
        update();
        if( canAnimate() )
            setBusy( true );
    }
    else if ( data.contains( "message" ) )
    {
        m_titleText = i18n( "Labels" ) + QString( " : " ) + data[ "message" ].toString();
        constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
        update();
        setBusy( false );
    }
    else if ( data.contains( "data" ) )
    {
        m_stoppedstate = false;
        m_titleText = i18n( "Labels for %1 - %2", data[ "artist" ].toString(), data[ "title" ].toString() );

        m_labelInfos = data[ "data" ].toMap();

        if( m_currentLabels.isEmpty() && m_autoAdd )
        {
            QMapIterator < QString, QVariant > it ( m_labelInfos );
            while( it.hasNext() )
            {
                it.next();
                if( !m_blacklist.contains( it.key() ) && it.value().toInt() >= m_minAutoAddCount )
                    toggleLabel( it.key() );
            }
        }
    
        updateLabels();

        setBusy( false );
    }
}

void
LabelsApplet::addLabelPressed()
{
    QString label = m_addLabel->currentText();

    if( label.isEmpty() )
        return;

    if( !m_currentLabels.contains( label ) )
    {
        toggleLabel( label );
        if( !m_allLabels.contains( label ) )
        {
            m_allLabels.append( label );
            m_allLabels.sort();
            m_addLabel->clear();
            m_addLabel->insertItems( 0, m_allLabels );
            m_addLabel->completionObject()->setItems( m_allLabels );
        }
        m_addLabel->clearEditText();
    }
}

void
LabelsApplet::toggleLabel( const QString &label )
{
    DEBUG_BLOCK
    bool selected;

    if( label.isEmpty() )
        return;
    
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( !track )
        return;
    
    // Inform collections of end of a metadata update
    Capabilities::UpdateCapability *uc = track->create<Capabilities::UpdateCapability>();
    if( !uc )
        return;

    Meta::LabelPtr labelPtr;
    
    foreach( const Meta::LabelPtr &labelIt, track->labels() )
    {
        if( label == labelIt->name() )
        {
            labelPtr = labelIt;
            break;
        }
    }
    
    if( m_currentLabels.contains( label ) )
    {
        track->removeLabel( labelPtr );
        m_currentLabels.removeAll( label );
        debug() << "removing label: " << label;
        selected = false;
    }
    else
    {
        track->addLabel( label );
        m_currentLabels.append( label );
        debug() << "adding label: " << label;
        selected = true;
    }
    uc->collectionUpdated();

    for( int i = 0; i < m_labelItems.count(); i++ )
    {
        if( m_labelItems[i]->toPlainText() == label )
        {
            m_labelItems[i]->setSelected( selected );
            return;
        }
    }
    
    updateLabels();
}

void
LabelsApplet::listLabel( const QString &label )
{
    DEBUG_BLOCK
    debug() << "listing tracks with label: " << label;

    AmarokUrl bookmark( "amarok://navigate/collections?filter=label:%22" + label + "%22" );
    bookmark.run();
}

void
LabelsApplet::blacklistLabel( const QString &label )
{
    DEBUG_BLOCK
    debug() << "blacklisting label: " << label;
    
    if( m_currentLabels.contains( label ) )
        toggleLabel( label );

    m_blacklist << label;
    KConfigGroup config = Amarok::config("Labels Applet");
    config.writeEntry( "Blacklist", m_blacklist );

    updateLabels();
}

void
LabelsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Labels Settings" ), "preferences-system");

    ui_Settings.minCountSpinBox->setValue( m_minCount );
    ui_Settings.numLabelsSpinBox->setValue( m_numLabels );
    ui_Settings.personalCountSpinBox->setValue( m_personalCount );
    ui_Settings.blacklistEditListBox->insertStringList( m_blacklist );
    ui_Settings.autoAddCheckBox->setChecked( m_autoAdd );
    ui_Settings.minAutoAddCountSpinBox->setValue( m_minAutoAddCount );
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}

void
LabelsApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Labels Applet");

    m_minCount = ui_Settings.minCountSpinBox->value();
    m_numLabels = ui_Settings.numLabelsSpinBox->value();
    m_personalCount = ui_Settings.personalCountSpinBox->value();
    m_autoAdd = ui_Settings.autoAddCheckBox->checkState() == Qt::Checked;
    m_minAutoAddCount = ui_Settings.minAutoAddCountSpinBox->value();
    m_blacklist = ui_Settings.blacklistEditListBox->items();
    config.writeEntry( "NumLabels", m_numLabels );
    config.writeEntry( "MinCount", m_minCount );
    config.writeEntry( "PersonalCount", m_personalCount );
    config.writeEntry( "AutoAdd", (int)m_autoAdd );
    config.writeEntry( "MinAutoAddCount", m_minAutoAddCount );
    config.writeEntry( "Blacklist", m_blacklist );

    dataEngine( "amarok-labels" )->query( QString( "labels:reload" ) );
}


#include "LabelsApplet.moc"
