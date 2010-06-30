/****************************************************************************************
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
 * Copyright (c) 2010 Hormiere Guillaume <hormiere.guillaume@gmail.com>                 *
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

#include "UpcomingEventsWidget.h"
#include "SvgHandler.h"

#include <KDateTime>
#include <KIcon>
#include <KLocale>

#include <Plasma/Label>

#include <QLabel>
#include <QPixmap>
#include <QGraphicsGridLayout>
#include <QGraphicsProxyWidget>

#define NBR_MAX_PARTICIPANT 5

UpcomingEventsWidget::UpcomingEventsWidget( QGraphicsItem *parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
{
    m_image = new QLabel;
    m_image->setAttribute( Qt::WA_NoSystemBackground );
    m_image->setAlignment( Qt::AlignCenter );
    m_image->setFixedSize( 128, 128 );

    QGraphicsProxyWidget *imageProxy = new QGraphicsProxyWidget;
    imageProxy->setWidget( m_image );
    m_participants = new Plasma::Label( this );
    m_date = new Plasma::Label( this );
    m_name = new Plasma::Label( this );
    m_url = new Plasma::Label( this );
    m_location = new Plasma::Label( this );
    m_participants->nativeWidget()->setWordWrap( true );
    m_name->nativeWidget()->setWordWrap( true );

    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    layout->addItem( imageProxy, 0, 0, 5, 1, Qt::AlignCenter );
    layout->addItem( m_name, 0, 1, 1, 1 );
    layout->addItem( m_participants, 1, 1, 1, 1 );
    layout->addItem( m_location, 2, 1, 1, 1 );
    layout->addItem( m_date, 3, 1, 1, 1 );
    layout->addItem( m_url, 4, 1, 1, 1 );
    layout->setColumnMinimumWidth( 0, 140 );
    setLayout( layout );

    m_url->nativeWidget()->setOpenExternalLinks( true );
    m_url->nativeWidget()->setTextInteractionFlags( Qt::TextBrowserInteraction );
}

UpcomingEventsWidget::~UpcomingEventsWidget()
{
}

Plasma::Label *
UpcomingEventsWidget::date() const
{
    return m_date;
}

const QPixmap *
UpcomingEventsWidget::image() const
{
    return m_image->pixmap();
}

Plasma::Label *
UpcomingEventsWidget::name() const
{
    return m_name;
}

Plasma::Label *
UpcomingEventsWidget::location() const
{
    return m_location;
}

Plasma::Label *
UpcomingEventsWidget::participants() const
{
    return m_participants;
}

Plasma::Label *
UpcomingEventsWidget::url() const
{
    return m_url;
}

void
UpcomingEventsWidget::setImage( const KUrl &url )
{
    if( !url.isValid() )
    {
        m_image->setPixmap( KIcon( "weather-none-available" ).pixmap( 120 ) );
        return;
    }

    m_image->setText( i18n("Loading picture...") );
    m_imageUrl = url;
    The::networkAccessManager()->getData( url, this,
         SLOT(loadImage(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
UpcomingEventsWidget::loadImage( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_imageUrl != url )
        return;

    m_imageUrl.clear();
    if( e.code != QNetworkReply::NoError )
        return;

    QPixmap image;
    if( image.loadFromData( data ) )
    {
        image = image.scaled( 116, 116, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        m_image->setPixmap( The::svgHandler()->addBordersToPixmap( image, 6, QString(), true ) );
    }
}

void
UpcomingEventsWidget::setParticipants( const QString &participants )
{
    QFont font;
    if( participants.isEmpty() )
    {
        m_participants->setText( i18n( "No other participants" ) );
        font.setItalic( true );
        m_participants->setFont( font );
    }
    else
    {
        QStringList listbuff = participants.split(" - ");
        QString buffer( "" );
        QString toolTipText;
        for( int i = 0; i < listbuff.size(); i++ )
        {
            toolTipText += listbuff.at( i );
            if( i % 3 == 0 && i )
            {
                toolTipText += "\n";
            }
            else
            {
                if( i < listbuff.size() - 1 ) toolTipText += " - ";
            }
                        
            if( i < NBR_MAX_PARTICIPANT )
            {
                buffer += listbuff.at( i );
                if( i < NBR_MAX_PARTICIPANT - 1 && i < listbuff.size()-1 ) buffer += " - ";
            }
        }
        if( listbuff.size() > NBR_MAX_PARTICIPANT )
        {
            buffer += "...";
        }
        m_participants->setToolTip( toolTipText );
        m_participants->setText( buffer );
        font.setItalic( false );
        m_participants->setFont( font );
    }
    m_participants->setAttribute( Qt::WA_TranslucentBackground );
}

void
UpcomingEventsWidget::setDate( const KDateTime &date )
{
    m_date->setText( KGlobal::locale()->formatDateTime( date.toClockTime(), KLocale::FancyLongDate ) );
    m_date->setAttribute( Qt::WA_TranslucentBackground );
}

void
UpcomingEventsWidget::setLocation( const QString &location )
{
    m_location->setText( location );
    m_location->setAttribute( Qt::WA_TranslucentBackground );
}

void
UpcomingEventsWidget::setName( const QString &name )
{
    QFont nameFont;
    nameFont.setBold( true );
    nameFont.setPointSize( m_name->font().pointSize() + 2 );
    m_name->setFont( nameFont );
    m_name->setText( name );
    m_name->setAttribute( Qt::WA_TranslucentBackground );
}

void
UpcomingEventsWidget::setUrl( const KUrl &url )
{
    m_url->setText( "<html><body><a href=\"" + url.prettyUrl() + "\"><u>" + i18n( "Event website" ) + "</u></a></body></html>" );
    m_url->setAttribute( Qt::WA_TranslucentBackground );
}

#include "UpcomingEventsWidget.moc"
