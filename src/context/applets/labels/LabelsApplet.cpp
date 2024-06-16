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

#define DEBUG_PREFIX "LabelsApplet"

#include "LabelsApplet.h"

#include "App.h"
#include "EngineController.h"
#include "PaletteHandler.h"
#include "amarokurls/AmarokUrl.h"
#include "context/Theme.h"
#include "context/applets/labels/LabelGraphicsItem.h"
#include "context/widgets/AppletHeader.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <Plasma/IconWidget>
#include <Plasma/Containment>

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KComboBox>

#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QPropertyAnimation>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#define LabelsAppletMaxLabelLength 40 // if a downloaded label is longer than this, don't show it

LabelsApplet::LabelsApplet( QObject *parent, const QVariantList &args )
    : Context::Applet( parent, args ),
    m_lastLabelSize( QSizeF(0,0) ),
    m_lastLabelBottomAdded( false )
{
    setHasConfigurationInterface( true );
}

LabelsApplet::~LabelsApplet()
{
    DEBUG_BLOCK
    qDeleteAll( m_labelItems );
    m_labelItems.clear();
    qDeleteAll( m_labelAnimations );
    m_labelAnimations.clear();
    qDeleteAll( m_labelItemsToDelete );
    m_labelItemsToDelete.clear();
    qDeleteAll( m_labelAnimationsToDelete );
    m_labelAnimationsToDelete.clear();

    if( m_reloadIcon )
        delete m_reloadIcon.data();
    if( m_settingsIcon )
        delete m_settingsIcon.data();
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
    enableHeader( true );
    setHeaderText( i18n( "Labels" ) );

    setCollapseHeight( m_header->height() );
    setMinimumHeight( collapseHeight() );

    // reload icon
    QAction *reloadAction = new QAction( this );
    reloadAction->setIcon( QIcon::fromTheme( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18n( "Reload" ) );
    m_reloadIcon = addLeftHeaderAction( reloadAction );
    m_reloadIcon.data()->setEnabled( false );
    connect( m_reloadIcon.data(), SIGNAL(clicked()), this, SLOT(reload()) );

    // settings icon
    QAction *settingsAction = new QAction( this );
    settingsAction->setIcon( QIcon::fromTheme( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addRightHeaderAction( settingsAction );
    connect( m_settingsIcon.data(), SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( m_header );

    m_addLabelProxy = new QGraphicsProxyWidget( this );
    m_addLabelProxy.data()->setAttribute( Qt::WA_NoSystemBackground );
    m_addLabel = new KComboBox( true );
    m_addLabel.data()->setAttribute( Qt::WA_NoSystemBackground );
    m_addLabel.data()->setAutoFillBackground( false );
    QPalette p = m_addLabel.data()->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlphaF( 0.4 );
    p.setColor( QPalette::Base, c );
    m_addLabel.data()->setPalette( p );
    m_addLabel.data()->completionObject()->setIgnoreCase( true );
    m_addLabel.data()->setCompletionMode( KCompletion::CompletionPopup );
    connect( m_addLabel.data(), SIGNAL(returnPressed()), this, SLOT(addLabelPressed()) );
    m_addLabelProxy.data()->setWidget( m_addLabel.data() );

    // Read config
    KConfigGroup config = Amarok::config("Labels Applet");
    m_minCount = config.readEntry( "MinCount", 30 );
    m_numLabels = config.readEntry( "NumLabels", 10 );
    m_personalCount = config.readEntry( "PersonalCount", 70 );
    m_autoAdd = config.readEntry( "AutoAdd", false );
    m_minAutoAddCount = config.readEntry( "MinAutoAddCount", 60 );
    m_selectedColor = config.readEntry( "SelectedColor", PaletteHandler::highlightColor( 2.0, 0.7 ) );
    const QPalette pal;
    m_backgroundColor = config.readEntry( "BackgroundColor", pal.color( QPalette::Base ) );

    m_matchArtist = config.readEntry( "MatchArtist", true );
    m_matchTitle = config.readEntry( "MatchTitle", true );
    m_matchAlbum = config.readEntry( "MatchAlbum", true );
    m_blacklist = config.readEntry( "Blacklist", QStringList() );

    const QStringList replacementList = config.readEntry( "ReplacementList", QStringList() );
    for( const QString &replacement : replacementList )
    {
        const QStringList parts = replacement.split( '|' );
        QString label = parts.at(0);
        label = label.replace( "%s", "|" );
        label = label.replace( "%p", "%" );
        QString replacementValue = parts.at(1);
        replacementValue = replacementValue.replace( "%s", "|" );
        replacementValue = replacementValue.replace( "%p", "%" );
        m_replacementMap.insert( label, replacementValue );
    }

    m_stoppedstate = false; // force an update
    setStoppedState( true );

    connectSource( "labels" );
    connect( dataEngine( "amarok-labels" ), SIGNAL(sourceAdded(QString)),
             this, SLOT(connectSource(QString)) );
}

void
LabelsApplet::setStoppedState( bool stopped )
{
    if( stopped == m_stoppedstate )
        return;

    m_stoppedstate = stopped;

    m_userLabels.clear();
    m_webLabels.clear();

    if( !stopped )
    {
        m_reloadIcon.data()->setEnabled( true );
        m_titleText = i18n( "Labels" );
        m_addLabelProxy.data()->show();
        m_addLabel.data()->show();
        m_addLabel.data()->clearEditText();
        // not needed at the moment, since setStoppedState(false) is only called in dataUpdated() and we know that the engine never sends state=started without the user labels;
        // so the minimum size is set in constraintsEvent()
//         setCollapseOffHeight( m_header->height() + m_addLabelProxy.data()->size().height() + 2 * standardPadding() );
//         setCollapseOff();
    }
    else
    {
        m_reloadIcon.data()->setEnabled( false );
        m_titleText = i18n( "Labels: No track playing" );
        m_addLabelProxy.data()->hide();
        m_addLabel.data()->hide();
        setBusy( false );
        qDeleteAll( m_labelItems );
        m_labelItems.clear();
        qDeleteAll( m_labelAnimations );
        m_labelAnimations.clear();
        setMinimumHeight( collapseHeight() );
        setCollapseOn();
    }
}

void
LabelsApplet::reload()
{
    DEBUG_BLOCK
    if( !m_stoppedstate )
        dataEngine( "amarok-labels" )->query( QString( "reload" ) );
}

void
LabelsApplet::animationFinished()
{
    if( QObject::sender() == 0 )
        return;

    for( int i=0; i<m_labelAnimations.count(); i++ )
    {
        if( QObject::sender() == m_labelAnimations.at(i) )
        {
            if( m_labelItems.at(i) )
                m_labelItems.at(i)->updateHoverStatus();
            m_labelAnimations.at(i)->setEasingCurve( QEasingCurve::InOutQuad );
            return;
        }
    }

    prepareGeometryChange();
    for( int i=0; i<m_labelAnimationsToDelete.count(); i++ )
    {
        if( QObject::sender() == m_labelAnimationsToDelete.at(i) )
        {
            delete m_labelItemsToDelete.at(i);
            delete m_labelAnimationsToDelete.at(i);
            m_labelItemsToDelete.removeAt(i);
            m_labelAnimationsToDelete.removeAt(i);
            return;
        }
    }
}

void
LabelsApplet::updateLabels()
{
    QMap < QString, int > tempLabelsMap;
    QMap < QString, int > finalLabelsMap;
    // holds all counts of web labels that are added to the final list
    QList < int > webCounts;

    // add the user assigned labels directly to the final map
    for( int i = 0; i < m_userLabels.count(); i++ )
    {
        finalLabelsMap.insert( m_userLabels.at( i ), m_personalCount );
    }
    // add the downloaded labels to the temp map first (if they aren't already in the final map / update value in final map if necessary)
    QMapIterator < QString, QVariant > it_infos ( m_webLabels );
    while( it_infos.hasNext() )
    {
        it_infos.next();
        if( !finalLabelsMap.contains( it_infos.key() )
            && it_infos.value().toInt() >= m_minCount
            && QString(it_infos.key()).length() <= LabelsAppletMaxLabelLength
            && !m_blacklist.contains( it_infos.key() )
            && !( m_matchArtist && QString(it_infos.key()).toLower() == m_artist.toLower() )
            && !( m_matchTitle && QString(it_infos.key()).toLower() == m_title.toLower() )
            && !( m_matchAlbum && QString(it_infos.key()).toLower() == m_album.toLower() ) )
        {
            tempLabelsMap.insert( it_infos.key(), it_infos.value().toInt() );
        }
        else if( finalLabelsMap.contains( it_infos.key() )
            && it_infos.value().toInt() > finalLabelsMap[ it_infos.key() ] )
        {
            finalLabelsMap[ it_infos.key() ] = it_infos.value().toInt();
            webCounts += it_infos.value().toInt();
        }
    }
    // then sort the values of the temp map
    QList < int > tempLabelsValues = tempLabelsMap.values();
    qSort( tempLabelsValues.begin(), tempLabelsValues.end(), qGreater < int > () );
    // and copy the highest rated labels to the final map until max. number is reached
    // if there are multiple items with equal low rating, move them to minList, sort it and add to the final map until max. number is reached
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
    // a lot of different values (73,68,51) is good, equal values (66,66,33) look suspicious
    // 0.7 / 0.3 is a pretty moderate choice; 0.5 / 0.5 would be more extreme
    const float qualityFactor = ( webCounts.count() > 0 ) ? 0.7 + 0.3 * (new QSet<int>(webCounts.begin(). webCounts.end())).count()/webCounts.count() : 1.0;
    // delete all unneeded label items
    for( int i=0; i<m_labelItems.count(); i++ )
    {
        if( !finalLabelsMap.contains( m_labelItems.at(i)->text() ) )
        {
            m_labelAnimations.at(i)->setEndValue( QPointF( size().width(), m_labelItems.at(i)->pos().y() ) );
            m_labelAnimations.at(i)->setEasingCurve( QEasingCurve::InQuad );
            m_labelAnimations.at(i)->start();
            m_labelItemsToDelete.append( m_labelItems.at(i) );
            m_labelAnimationsToDelete.append( m_labelAnimations.at(i) );
            m_labelItems.removeAt(i);
            m_labelAnimations.removeAt(i);
            i--;
        }
    }

    // and finally create the LabelGraphicsItems
    // add them to a temp list first, so they are in the same order as the final label items map (sorted alphabetically)
    QList < LabelGraphicsItem * > tempLabelItems;
    QList < QPropertyAnimation * > tempLabelAnimations;
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

        const qreal f_size = qMax( adjustedCount / 10.0 - 5.0, -2.0 );

        LabelGraphicsItem *labelGraphics = 0;
        QPropertyAnimation *labelAnimation = 0;
        for( int i=0; i<m_labelItems.count(); i++ )
        {
            if( m_labelItems.at(i)->text() == it_final.key() )
            {
                labelGraphics = m_labelItems.at(i);
                labelGraphics->setDeltaPointSize( f_size );
                labelGraphics->setSelected( m_userLabels.contains( it_final.key() ) );
                if( !m_lastLabelSize.isEmpty() && m_lastLabelName == m_labelItems.at(i)->text() )
                {
                    const qreal x = labelGraphics->pos().x() - ( labelGraphics->boundingRect().width()  - m_lastLabelSize.width()  ) / 2;
                    const qreal y = labelGraphics->pos().y() - ( labelGraphics->boundingRect().height() - m_lastLabelSize.height() ) / 2;
                    labelGraphics->setPos( x, y );
                    m_lastLabelSize = QSizeF( 0, 0 );
                    m_lastLabelName.clear();
                }
                labelAnimation = m_labelAnimations.at(i);
                break;
            }
        }
        if( !labelGraphics )
        {
            labelGraphics = new LabelGraphicsItem( it_final.key(), f_size, this );
            labelGraphics->setSelectedColor( m_selectedColor );
            labelGraphics->setBackgroundColor( m_backgroundColor );
            labelGraphics->showBlacklistButton( !m_allLabels.contains(it_final.key()) );
            labelGraphics->setSelected( m_userLabels.contains( it_final.key() ) );
            if( m_lastLabelBottomAdded )
            {
                labelGraphics->setPos( m_addLabelProxy.data()->pos().x(), m_addLabelProxy.data()->pos().y() + m_addLabelProxy.data()->size().height()/2 - labelGraphics->boundingRect().height()/2 );
                m_lastLabelBottomAdded = false;
            }
            connect( labelGraphics, SIGNAL(toggled(QString)), SLOT(toggleLabel(QString)) );
            connect( labelGraphics, SIGNAL(list(QString)), SLOT(listLabel(QString)) );
            connect( labelGraphics, SIGNAL(blacklisted(QString)), SLOT(blacklistLabel(QString)) );

            labelAnimation = new QPropertyAnimation( labelGraphics, "pos" );
            labelAnimation->setEasingCurve( QEasingCurve::OutQuad );
            connect( labelAnimation, SIGNAL(finished()), this, SLOT(animationFinished()) );
        }
        tempLabelItems.append( labelGraphics );
        tempLabelAnimations.append( labelAnimation );
    }
    // copy the temp list to the final list
    m_labelItems = tempLabelItems;
    m_labelAnimations = tempLabelAnimations;

    // should be unnecessary, but better safe than sorry
    m_lastLabelName.clear();
    m_lastLabelSize = QSizeF( 0, 0 );
    m_lastLabelBottomAdded = false;

    constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
}

void
LabelsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );

    setHeaderText( m_titleText );

    if( !m_stoppedstate )
    {
        const qreal horzontalPadding = standardPadding() / 2;
        const qreal verticalPadding = standardPadding() / 2;
        qreal x_pos;
        qreal y_pos = m_header->boundingRect().bottom() + 1.5 * standardPadding();
        qreal width = 0;
        qreal height = 0;
        int start_index = 0;
        int end_index = -1;
        qreal max_width = size().width() - 2 * standardPadding();
        for( int i = 0; i < m_labelItems.count(); i++ )
        {
            QRectF l_size = m_labelItems.at(i)->boundingRect();
            if( width + l_size.width() + horzontalPadding <= max_width || i == 0 )
            {
                width += l_size.width();
                if( i != 0 )
                    width += horzontalPadding;
                if( l_size.height() > height )
                    height = l_size.height();
                end_index = i;
            }
            else
            {
                x_pos = ( max_width - width ) / 2 + standardPadding();
                for( int j = start_index; j <= end_index; j++ )
                {
                    const QRectF c_size = m_labelItems.at(j)->boundingRect();
                    const QPointF pos = QPointF( x_pos, y_pos + (height-c_size.height())/2 );
                    if( m_labelItems.at(j)->pos() == QPointF(0,0) )
                        m_labelItems.at(j)->setPos( -c_size.width(), pos.y() );
                    m_labelAnimations.at(j)->setEndValue( pos );
                    if( m_labelAnimations.at(j)->state() != QAbstractAnimation::Running )
                        m_labelAnimations.at(j)->start();
                    m_labelItems.at(j)->updateHoverStatus();
                    x_pos += c_size.width() + horzontalPadding;
                }
                y_pos += height + verticalPadding;
                width = l_size.width();
                height = l_size.height();
                start_index = i;
                end_index = i;
            }
        }
        x_pos = ( max_width - width ) / 2 + standardPadding();
        for( int j = start_index; j <= end_index; j++ )
        {
            const QRectF c_size = m_labelItems.at(j)->boundingRect();
            const QPointF pos = QPointF( x_pos, y_pos + (height-c_size.height())/2 );
            if( m_labelItems.at(j)->pos() == QPointF(0,0) )
                m_labelItems.at(j)->setPos( -c_size.width(), pos.y() );
            m_labelAnimations.at(j)->setEndValue( pos );
            if( m_labelAnimations.at(j)->state() != QAbstractAnimation::Running )
                m_labelAnimations.at(j)->start();
            m_labelItems.at(j)->updateHoverStatus();
            x_pos += c_size.width() + horzontalPadding;
        }
        if( m_labelItems.count() > 0 )
            y_pos += height + standardPadding();

        const qreal addLabelProxyWidth = qMin( size().width() - 2 * standardPadding(), (qreal)300.0 );
        m_addLabelProxy.data()->setPos( ( size().width() - addLabelProxyWidth ) / 2, y_pos );
        m_addLabelProxy.data()->setMinimumWidth( addLabelProxyWidth );
        m_addLabelProxy.data()->setMaximumWidth( addLabelProxyWidth );
        y_pos += m_addLabelProxy.data()->size().height() + standardPadding();

        setMinimumHeight( y_pos );

        setCollapseOffHeight( y_pos );
        setCollapseOff();
    }
}

void
LabelsApplet::connectSource( const QString &source )
{
    if( source == "labels" )
        dataEngine( "amarok-labels" )->connectSource( "labels", this );
}

void
LabelsApplet::dataUpdated( const QString &name, const Plasma::DataEngine::Data &data ) // SLOT
{
    Q_UNUSED( name )

    if( data.isEmpty() )
        return;

    if( data.contains( "state" ) && data["state"].toString().contains("started") )
        setStoppedState( false );
    else if( data.contains( "state" ) && data["state"].toString().contains("stopped") )
        setStoppedState( true );

    if( data.contains( "message" ) && data["message"].toString().contains("fetching") )
    {
        m_titleText = i18n( "Labels: Fetching..." );
        if ( !data.contains( "user" ) ) // avoid calling update twice
        {
            constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
        }
        if( canAnimate() )
            setBusy( true );
    }
    else if( data.contains( "message" ) )
    {
        m_titleText = i18n( "Labels: %1", data[ "message" ].toString() );
        if( !data.contains( "user" ) ) // avoid calling update twice
        {
            constraintsEvent(); // don't use updateConstraints() in order to avoid labels displayed at pos. 0,0 for a moment
        }
        setBusy( false );
    }

    if( data.contains( "artist" ) )
        m_artist = data[ "artist" ].toString();

    if( data.contains( "title" ) )
        m_title = data[ "title" ].toString();

    if( data.contains( "album" ) )
        m_album = data[ "album" ].toString();

    if( data.contains( "all" ) )
    {
        m_allLabels = data[ "all" ].toStringList();
        m_allLabels.sort();

        const QString saveText = m_addLabel.data()->lineEdit()->text();
        m_addLabel.data()->clear();
        m_addLabel.data()->insertItems( 0, m_allLabels );
        m_addLabel.data()->completionObject()->setItems( m_allLabels );
        m_addLabel.data()->lineEdit()->setText( saveText );
    }

    if( data.contains( "user" ) )
    {
//         debug() << "new user labels:" << data[ "user" ].toStringList().join(", ");
        if( !m_stoppedstate ) // otherwise there's been an error
        {
            m_userLabels = data[ "user" ].toStringList();
            m_webLabels.clear(); // we can safely clear the web labels because user labels will never be updated without the web labels

            if( !data.contains( "web" ) ) // avoid calling updateLabels twice
                updateLabels();
        }
    }

    if( data.contains( "web" ) )
    {
//         debug() << "new web labels:" << QStringList(data[ "web" ].toMap().keys()).join(", ");
        if( !m_stoppedstate ) // otherwise there's been an error
        {
            if( !data.contains( "message" ) )
                m_titleText = i18n( "Labels for %1 by %2", m_title, m_artist );

            setBusy( false );

            m_webLabels = data[ "web" ].toMap();

            // rename/merge web labels if they are present in the replacement map
            QMap < QString, QVariant >::iterator it = m_webLabels.begin();
            while( it != m_webLabels.end() )
            {
                if( m_replacementMap.contains(it.key()) )
                {
                    const QString replacement = m_replacementMap.value( it.key() );
                    if( m_webLabels.contains(replacement) ) // we have to merge
                    {
                        m_webLabels[replacement] = qMin( 100, it.value().toInt() + m_webLabels.value(replacement).toInt() );
                        it = m_webLabels.erase( it );
                    }
                    else // just replace
                    {
                        const int count = it.value().toInt();
                        it = m_webLabels.erase( it );
                        m_webLabels.insert( replacement, count );
                    }
                }
                else
                {
                    ++it;
                }
            }

            // auto add labels if needed
            if( m_userLabels.isEmpty() && m_autoAdd )
            {
                QMapIterator < QString, QVariant > it ( m_webLabels );
                while( it.hasNext() )
                {
                    it.next();
                    if( it.value().toInt() >= m_minAutoAddCount
                        && QString(it.key()).length() <= LabelsAppletMaxLabelLength
                        && !m_blacklist.contains( it.key() )
                        && !( m_matchArtist && QString(it.key()).toLower() == m_artist.toLower() )
                        && !( m_matchTitle && QString(it.key()).toLower() == m_title.toLower() )
                        && !( m_matchAlbum && QString(it.key()).toLower() == m_album.toLower() ) )
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
    const QString label = m_addLabel.data()->currentText();

    if( label.isEmpty() )
        return;

    if( !m_userLabels.contains( label ) )
    {
        toggleLabel( label );
        m_addLabel.data()->clearEditText();
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

    Meta::LabelPtr labelPtr;

    for( const Meta::LabelPtr &labelIt : track->labels() )
    {
        if( label == labelIt->name() )
        {
            labelPtr = labelIt;
            break;
        }
    }

    for( int i=0; i<m_labelItems.count(); i++ )
    {
        if( m_labelItems.at(i)->text() == label )
        {
            m_lastLabelSize = m_labelItems.at(i)->boundingRect().size();
            m_lastLabelName = label;
            break;
        }
    }

    if( m_userLabels.contains( label ) )
    {
        track->removeLabel( labelPtr );
        m_userLabels.removeAll( label );
        debug() << "removing label: " << label;
    }
    else
    {
        track->addLabel( label );
        m_userLabels.append( label );
        debug() << "adding label: " << label;
        m_lastLabelBottomAdded = true;
    }

    if( !m_allLabels.contains( label ) )
    {
        m_allLabels.append( label );
        m_allLabels.sort();

        const QString saveText = m_addLabel.data()->lineEdit()->text();
        m_addLabel.data()->clear();
        m_addLabel.data()->insertItems( 0, m_allLabels );
        m_addLabel.data()->completionObject()->setItems( m_allLabels );
        m_addLabel.data()->lineEdit()->setText( saveText );
    }

    // usually the engine keeps track of label changes of the playing track
    // (except if the labels get auto added, this is why we have to keep m_userLabels up to date)
    // but it doesn't work always, so we update
    updateLabels();
}

void
LabelsApplet::listLabel( const QString &label )
{
    AmarokUrl bookmark( "amarok://navigate/collections?filter=label:" + AmarokUrl::escape( "=" ) + "%22" + AmarokUrl::escape( label ) + "%22" );
    bookmark.run();
}

void
LabelsApplet::blacklistLabel( const QString &label )
{
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

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    parent->setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    parent->connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    parent->connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    KConfigGroup configuration = config();
    QWidget *generalSettings = new QWidget;
    ui_GeneralSettings.setupUi( generalSettings );
    ui_GeneralSettings.resetColorsPushButton->setIcon( QIcon::fromTheme("fill-color") );
    QWidget *blacklistSettings = new QWidget;
    ui_BlacklistSettings.setupUi( blacklistSettings );
    QWidget *replacementSettings = new QWidget;
    ui_ReplacementSettings.setupUi( replacementSettings );
    ui_ReplacementSettings.addPushButton->setIcon( QIcon::fromTheme("list-add") );
    ui_ReplacementSettings.removePushButton->setIcon( QIcon::fromTheme("list-remove") );

    parent->addPage( generalSettings, i18n( "General Settings" ), "preferences-system" );
    parent->addPage( blacklistSettings, i18n( "Blacklist Settings" ), "flag-black" );
    parent->addPage( replacementSettings, i18n( "Replacement Settings" ), "system-search" );

    ui_GeneralSettings.minCountSpinBox->setValue( m_minCount );
    ui_GeneralSettings.numLabelsSpinBox->setValue( m_numLabels );
    ui_GeneralSettings.personalCountSpinBox->setValue( m_personalCount );
    ui_GeneralSettings.autoAddCheckBox->setChecked( m_autoAdd );
    ui_GeneralSettings.minAutoAddCountSpinBox->setValue( m_minAutoAddCount );
    ui_GeneralSettings.selectedColorButton->setColor( m_selectedColor );
    ui_GeneralSettings.backgroundColorButton->setColor( m_backgroundColor );

    ui_BlacklistSettings.matchArtistCheckBox->setChecked( m_matchArtist );
    ui_BlacklistSettings.matchTitleCheckBox->setChecked( m_matchTitle );
    ui_BlacklistSettings.matchAlbumCheckBox->setChecked( m_matchAlbum );
    ui_BlacklistSettings.blacklistEditListBox->insertStringList( m_blacklist );

    QHashIterator < QString, QString > it ( m_replacementMap );
    while( it.hasNext() )
    {
        it.next();
        new QTreeWidgetItem( ui_ReplacementSettings.replacementTreeWidget, QStringList() << it.key() << it.value() );
    }

    connect( ui_GeneralSettings.resetColorsPushButton, SIGNAL(clicked()), this, SLOT(settingsResetColors()) );
    connect( ui_ReplacementSettings.addPushButton, SIGNAL(clicked()), this, SLOT(settingsAddReplacement()) );
    connect( ui_ReplacementSettings.removePushButton, SIGNAL(clicked()), this, SLOT(settingsRemoveReplacement()) );
    connect( parent, SIGNAL(accepted()), this, SLOT(saveSettings()) );
}

void
LabelsApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Labels Applet");

    m_minCount = ui_GeneralSettings.minCountSpinBox->value();
    m_numLabels = ui_GeneralSettings.numLabelsSpinBox->value();
    m_personalCount = ui_GeneralSettings.personalCountSpinBox->value();
    m_autoAdd = ui_GeneralSettings.autoAddCheckBox->checkState() == Qt::Checked;
    m_minAutoAddCount = ui_GeneralSettings.minAutoAddCountSpinBox->value();
    m_selectedColor = ui_GeneralSettings.selectedColorButton->color();
    m_backgroundColor = ui_GeneralSettings.backgroundColorButton->color();

    m_matchArtist = ui_BlacklistSettings.matchArtistCheckBox->checkState() == Qt::Checked;
    m_matchTitle = ui_BlacklistSettings.matchTitleCheckBox->checkState() == Qt::Checked;
    m_matchAlbum = ui_BlacklistSettings.matchAlbumCheckBox->checkState() == Qt::Checked;
    m_blacklist = ui_BlacklistSettings.blacklistEditListBox->items();

    m_replacementMap.clear();
    for( int i=0; i<ui_ReplacementSettings.replacementTreeWidget->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem *item = ui_ReplacementSettings.replacementTreeWidget->topLevelItem( i );
        m_replacementMap.insert( item->text(0), item->text(1) );
    }

    config.writeEntry( "NumLabels", m_numLabels );
    config.writeEntry( "MinCount", m_minCount );
    config.writeEntry( "PersonalCount", m_personalCount );
    config.writeEntry( "AutoAdd", m_autoAdd );
    config.writeEntry( "MinAutoAddCount", m_minAutoAddCount );
    config.writeEntry( "SelectedColor", m_selectedColor );
    config.writeEntry( "BackgroundColor", m_backgroundColor );

    config.writeEntry( "MatchArtist", m_matchArtist );
    config.writeEntry( "MatchTitle", m_matchTitle );
    config.writeEntry( "MatchAlbum", m_matchAlbum );
    config.writeEntry( "Blacklist", m_blacklist );

    QStringList replacementList;
    QHashIterator < QString, QString > it ( m_replacementMap );
    while( it.hasNext() )
    {
        it.next();
        QString label = it.key();
        label = label.replace( '%', "%p" );
        label = label.replace( '|', "%s" );
        QString replacement = it.value();
        replacement = replacement.replace( '%', "%p" );
        replacement = replacement.replace( '|', "%s" );
        replacementList.append( label + '|' + replacement );
    }
    config.writeEntry( "ReplacementList", replacementList );

    for( int i=0; i<m_labelItems.count(); i++ )
    {
        m_labelItems.at(i)->setSelectedColor( m_selectedColor );
        m_labelItems.at(i)->setBackgroundColor( m_backgroundColor );
    }

    reload();
}

void
LabelsApplet::settingsResetColors()
{
    ui_GeneralSettings.selectedColorButton->setColor( PaletteHandler::highlightColor( 2.0, 0.7 ) );
    const QPalette pal;
    ui_GeneralSettings.backgroundColorButton->setColor( pal.color( QPalette::Base ) );
}

void
LabelsApplet::settingsAddReplacement()
{
    const QString label = ui_ReplacementSettings.labelLineEdit->text();
    const QString replacement = ui_ReplacementSettings.replacementLineEdit->text();

    if( label.isEmpty() || replacement.isEmpty() )
        return;

    new QTreeWidgetItem( ui_ReplacementSettings.replacementTreeWidget, QStringList() << label << replacement );
    ui_ReplacementSettings.labelLineEdit->clear();
    ui_ReplacementSettings.replacementLineEdit->clear();
}

void
LabelsApplet::settingsRemoveReplacement()
{
    for( int i=0; i<ui_ReplacementSettings.replacementTreeWidget->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem *item = ui_ReplacementSettings.replacementTreeWidget->topLevelItem( i );
        if( item->isSelected() )
        {
            ui_ReplacementSettings.replacementTreeWidget->takeTopLevelItem( i );
            i--;
        }
    }
}


