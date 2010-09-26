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

#include "App.h"
#include "Theme.h"
#include "EngineController.h"
#include "core/support/Debug.h"
#include "core/capabilities/UpdateCapability.h"
#include "context/widgets/TextScrollingWidget.h"
#include "amarokurls/AmarokUrl.h"

#include <Plasma/IconWidget>
#include <Plasma/Containment>

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KComboBox>

#include <QGraphicsProxyWidget>


LabelsApplet::LabelsApplet( QObject *parent, const QVariantList &args )
    : Context::Applet( parent, args )
{
    setHasConfigurationInterface( true );
}

LabelsApplet::~LabelsApplet()
{
    DEBUG_BLOCK
    qDeleteAll( m_labelItems );
    m_labelItems.clear();

    if( m_reloadIcon )
        delete m_reloadIcon;
    if( m_settingsIcon )
        delete m_settingsIcon;
}

void
LabelsApplet::init()
{
    DEBUG_BLOCK

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

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Labels Applet");
    m_minCount = config.readEntry( "MinCount", 30 );
    m_numLabels = config.readEntry( "NumLabels", 10 );
    m_personalCount = config.readEntry( "PersonalCount", 70 );
    m_autoAdd = config.readEntry( "AutoAdd", false );
    m_minAutoAddCount = config.readEntry( "MinAutoAddCount", 60 );
    m_matchArtist = config.readEntry( "MatchArtist", true );
    m_matchTitle = config.readEntry( "MatchTitle", true );
    QStringList defaultBlacklist;
    defaultBlacklist << "favourite" << "like it";
    m_blacklist = config.readEntry( "Blacklist", defaultBlacklist );

    setStoppedState( true );

    connectSource( "labels" );
    connect( dataEngine( "amarok-labels" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );
}

void LabelsApplet::setStoppedState( bool stopped )
{
    DEBUG_BLOCK
    
    if( stopped == m_stoppedstate )
        return;
    
    m_stoppedstate = stopped;

    m_userLabels.clear();
    m_webLabels.clear();

    if( !stopped )
    {
        m_reloadIcon->setEnabled( true );
        m_titleText = i18n( "Labels" );
        m_addLabelProxy->show();
        m_addLabel->clearEditText();
    }
    else
    {
        m_reloadIcon->setEnabled( false );
        m_titleText = i18n( "Labels" ) + QString( " : " ) + i18n( "No track playing" );
        m_addLabelProxy->hide();
        setBusy( false );
        setMinimumHeight( 0 );
        setCollapseOn();
        qDeleteAll( m_labelItems );
        m_labelItems.clear();
    }

    constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
    update();
}

void
LabelsApplet::reload()
{
    DEBUG_BLOCK
    if( !m_stoppedstate )
    {
        dataEngine( "amarok-labels" )->query( QString( "reload" ) );
    }
}

void
LabelsApplet::updateLabels()
{
    DEBUG_BLOCK
    qDeleteAll( m_labelItems );
    m_labelItems.clear();
    
    QMap < QString, int > tempLabelsMap;
    QMap < QString, int > finalLabelsMap;
    // holds all counts of web labels that are added to the final list
    QList < int > webCounts;

    // add the user assigned labels directly to the final map
    for( int i = 0; i < m_userLabels.count(); i++ )
    {
        finalLabelsMap.insert( m_userLabels.at( i ), m_personalCount );
        qDebug() << "LabelsApplet:" << "user label:" << m_userLabels.at( i ) << "count:" << m_personalCount;
    }
    // add the downloaded labels to the temp map first (if they aren't alreday in the final map / update value in final map if necessary)
    QMapIterator < QString, QVariant > it_infos ( m_webLabels );
    while( it_infos.hasNext() )
    {
        it_infos.next();
        if( !finalLabelsMap.contains( it_infos.key() ) && !m_blacklist.contains( it_infos.key() ) && it_infos.value().toInt() >= m_minCount
            && QString(it_infos.key()).length() <= 40 && !( m_matchArtist && QString(it_infos.key()).toLower() == m_artist.toLower() )  && !( m_matchTitle && QString(it_infos.key()).toLower() == m_title.toLower() ) )
        {
            tempLabelsMap.insert( it_infos.key(), it_infos.value().toInt() );
        }
        else if( finalLabelsMap.contains( it_infos.key() ) && it_infos.value().toInt() > finalLabelsMap[ it_infos.key() ] )
        {
            finalLabelsMap[ it_infos.key() ] = it_infos.value().toInt();
            webCounts += it_infos.value().toInt();
        }
        qDebug() << "LabelsApplet:" << "web label:" << it_infos.key() << "count:" << it_infos.value().toInt();
    }
    // then sort the values of the temp map
    QList < int > tempLabelsValues = tempLabelsMap.values();
    qSort( tempLabelsValues.begin(), tempLabelsValues.end(), qGreater < int > () );
    // and copy the highest rated labels to the final map until max. number is reached
    const int additionalNum = m_numLabels - finalLabelsMap.count();
    if( additionalNum > 0 && tempLabelsValues.count() > 0 )
    {
        int minCount;
        QStringList minList;
        if( additionalNum <= tempLabelsValues.count() )
            minCount = tempLabelsValues.at( additionalNum - 1 );
        else
            minCount = tempLabelsValues.last();
        QMapIterator < QString, int > it_temp ( tempLabelsMap );
        while( it_temp.hasNext() )
        {
            it_temp.next();
            if( it_temp.value() > minCount )
            {
                finalLabelsMap.insert( it_temp.key(), it_temp.value() );
                webCounts += it_temp.value();
            }
            else if( it_temp.value() == minCount )
            {
                minList += it_temp.key();
            }
        }
        minList.sort();
        while( minList.count() > 0 && finalLabelsMap.count() < m_numLabels )
        {
            finalLabelsMap.insert( minList.first(), minCount );
            webCounts += minCount;
            minList.takeFirst();
        }
    }
    // now make the label cloud nicer by determinating the quality of the web labels
    // 0.7 / 0.3 is a pretty moderate choice; 0.5 / 0.5 would be more extreme
    const float qualityFactor = ( webCounts.count() > 0 ) ? 0.7 + 0.3 * webCounts.toSet().count()/webCounts.count() : 1.0;
    qDebug() << "LabelsApplet:" << "qualityFactor:" << qualityFactor;
    // and finally create the LabelGraphicsItems
    QMapIterator < QString, int > it_final ( finalLabelsMap );
    while( it_final.hasNext() )
    {
        it_final.next();
        
        if( it_final.key().isEmpty() ) // empty labels don't make sense but they cause a freeze
            continue;

        // quality of web labels adjusted value
        int adjustedCount = qualityFactor * it_final.value();
        if( m_userLabels.contains( it_final.key() ) && adjustedCount < m_personalCount )
            adjustedCount = m_personalCount;
        
        qreal f_size = adjustedCount / 10 - 5;
        if( f_size < -2 )
            f_size = -2;

        qDebug() << "LabelsApplet:" << "final label:" << it_final.key() << "count:" << adjustedCount;

        LabelGraphicsItem *labelGraphics = new LabelGraphicsItem( it_final.key(), f_size, this );
        if( m_userLabels.contains( it_final.key() ) )
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
    DEBUG_BLOCK
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
                    m_labelItems.at(j)->setPos( x_pos, y_pos + (height-c_size.height())/2 );
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
            m_labelItems.at(j)->setPos( x_pos, y_pos + (height-c_size.height())/2 );
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
    DEBUG_BLOCK
    Q_UNUSED( name )

    if ( data.isEmpty() )
        return;

    if ( data.contains( "state" ) && data["state"].toString().contains("started") )
    {
        qDebug() << "LabelsApplet:" << "started";
        setStoppedState( false );
    }
    else if ( data.contains( "state" ) && data["state"].toString().contains("stopped") )
    {
        qDebug() << "LabelsApplet:" << "stopped";
        setStoppedState( true );
    }
    
    if ( data.contains( "message" ) && data["message"].toString().contains("fetching") )
    {
        qDebug() << "LabelsApplet:" << "fetching";
        m_titleText = i18n( "Labels" ) + QString( " : " ) + i18n( "Fetching ..." );
        constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
        update();
        if( canAnimate() )
            setBusy( true );
    }
    else if ( data.contains( "message" ) )
    {
        qDebug() << "LabelsApplet:" << "mssage:" << data[ "message" ].toString();
        m_titleText = i18n( "Labels" ) + QString( " : " ) + data[ "message" ].toString();
        constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
        update();
        setBusy( false );
    }

    if ( data.contains( "artist" ) )
    {
        qDebug() << "LabelsApplet:" << "artist";
        m_artist = data[ "artist" ].toString();
    }
    
    if ( data.contains( "title" ) )
    {
        qDebug() << "LabelsApplet:" << "title";
        m_title = data[ "title" ].toString();
    }
    
    if ( data.contains( "all" ) )
    {
        qDebug() << "LabelsApplet:" << "all";
        m_allLabels = data[ "all" ].toStringList();
        m_allLabels.sort();

        const QString saveText = m_addLabel->lineEdit()->text();
        m_addLabel->clear();
        m_addLabel->insertItems( 0, m_allLabels );
        m_addLabel->completionObject()->setItems( m_allLabels );
        m_addLabel->lineEdit()->setText( saveText );
    }

    if ( data.contains( "user" ) )
    {
        qDebug() << "LabelsApplet:" << "user";
        if( !m_stoppedstate ) // otherwise there's been an error
        {
            m_userLabels = data[ "user" ].toStringList();
            m_webLabels.clear(); // we can saftly clear the web labels because user labels will never be updated without the web labels

            if ( !data.contains( "web" ) ) // avoid calling updateLabels twice
                updateLabels();
        }
    }
    
    if ( data.contains( "web" ) )
    {
        qDebug() << "LabelsApplet:" << "web";
        if( !m_stoppedstate ) // otherwise there's been an error
        {
            m_titleText = i18n( "Labels for %1 by %2", m_title, m_artist );
            setBusy( false );

            m_webLabels = data[ "web" ].toMap();

            if( m_userLabels.isEmpty() && m_autoAdd )
            {
                QMapIterator < QString, QVariant > it ( m_webLabels );
                while( it.hasNext() )
                {
                    it.next();
                    if( !m_blacklist.contains( it.key() ) && it.value().toInt() >= m_minAutoAddCount )
                        toggleLabel( it.key() );
                }
            }

            updateLabels();
        }
    }
}

void
LabelsApplet::addLabelPressed()
{
    const QString label = m_addLabel->currentText();

    if( label.isEmpty() )
        return;

    if( !m_userLabels.contains( label ) )
    {
        toggleLabel( label );
        m_addLabel->clearEditText();
    }
}

void
LabelsApplet::toggleLabel( const QString &label )
{
    DEBUG_BLOCK

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
    
    if( m_userLabels.contains( label ) )
    {
        track->removeLabel( labelPtr );
        debug() << "LabelsApplet:" << "removing label: " << label;
    }
    else
    {
        track->addLabel( label );
        debug() << "LabelsApplet:" << "adding label: " << label;
    }
    uc->collectionUpdated();

    // no need to update the label cloud since the engine keeps track of label changes of the playing track

    if( !m_allLabels.contains( label ) )
    {
        m_allLabels.append( label );
        m_allLabels.sort();
        
        const QString saveText = m_addLabel->lineEdit()->text();
        m_addLabel->clear();
        m_addLabel->insertItems( 0, m_allLabels );
        m_addLabel->completionObject()->setItems( m_allLabels );
        m_addLabel->lineEdit()->setText( saveText );
    }
}

void
LabelsApplet::listLabel( const QString &label )
{
    DEBUG_BLOCK
    debug() << "LabelsApplet:" << "listing tracks with label: " << label;

    AmarokUrl bookmark( "amarok://navigate/collections?filter=label:%22" + label + "%22" );
    bookmark.run();
}

void
LabelsApplet::blacklistLabel( const QString &label )
{
    DEBUG_BLOCK
    debug() << "LabelsApplet:" << "blacklisting label: " << label;
    
    if( m_userLabels.contains( label ) )
        toggleLabel( label );

    m_blacklist << label;
    KConfigGroup config = Amarok::config("Labels Applet");
    config.writeEntry( "Blacklist", m_blacklist );

    updateLabels();
}

void
LabelsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    DEBUG_BLOCK
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Labels Settings" ), "preferences-system");

    ui_Settings.minCountSpinBox->setValue( m_minCount );
    ui_Settings.numLabelsSpinBox->setValue( m_numLabels );
    ui_Settings.personalCountSpinBox->setValue( m_personalCount );
    ui_Settings.autoAddCheckBox->setChecked( m_autoAdd );
    ui_Settings.minAutoAddCountSpinBox->setValue( m_minAutoAddCount );
    ui_Settings.matchArtistCheckBox->setChecked( m_matchArtist );
    ui_Settings.matchTitleCheckBox->setChecked( m_matchTitle );
    ui_Settings.blacklistEditListBox->insertStringList( m_blacklist );
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
    m_matchArtist = ui_Settings.matchArtistCheckBox->checkState() == Qt::Checked;
    m_matchTitle = ui_Settings.matchTitleCheckBox->checkState() == Qt::Checked;
    m_blacklist = ui_Settings.blacklistEditListBox->items();
    config.writeEntry( "NumLabels", m_numLabels );
    config.writeEntry( "MinCount", m_minCount );
    config.writeEntry( "PersonalCount", m_personalCount );
    config.writeEntry( "AutoAdd", m_autoAdd );
    config.writeEntry( "MinAutoAddCount", m_minAutoAddCount );
    config.writeEntry( "MatchArtist", m_matchArtist );
    config.writeEntry( "MatchTitle", m_matchTitle );
    config.writeEntry( "Blacklist", m_blacklist );

    if( !m_stoppedstate )
    {
        dataEngine( "amarok-labels" )->query( QString( "reload" ) );
    }
}


#include "LabelsApplet.moc"
