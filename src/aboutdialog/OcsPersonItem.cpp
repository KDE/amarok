/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "OcsPersonItem.h"

#include "core/support/Debug.h"

#include <QAction>
#include <KRun>
#include <QStandardPaths>
#include <Attica/Provider>
#include <Attica/ItemJob>
#include <KIO/OpenUrlJob>
#include <KIO/StoredTransferJob>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyleOption>

OcsPersonItem::OcsPersonItem( const KAboutPerson &person, const QString &ocsUsername, PersonStatus status, QWidget *parent )
    : QWidget( parent )
    , m_status( status )
    , m_state( Offline )
{
    m_person = &person;
    m_ocsUsername = ocsUsername;

    setupUi( this );
    init();

    m_avatar->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    //TODO: Add favorite artists!

}

void
OcsPersonItem::init()
{
    m_textLabel->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_textLabel->setOpenExternalLinks( true );
    m_textLabel->setContentsMargins( 5, 0, 0, 2 );
    m_verticalLayout->setSpacing( 0 );

    m_vertLine->hide();
    m_initialSpacer->changeSize( 0, 40, QSizePolicy::Fixed, QSizePolicy::Fixed );
    layout()->invalidate();

    m_aboutText.append( "<b>" + m_person->name() + "</b>" );
    if( !m_person->task().isEmpty() )
        m_aboutText.append( "<br/>" + m_person->task() );

    m_iconsBar = new KToolBar( this, false, false );
    m_snBar = new KToolBar( this, false, false );

    m_iconsBar->setIconSize( QSize( 22, 22 ) );
    m_iconsBar->setContentsMargins( 0, 0, 0, 0 );
    m_iconsBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

    if( m_status == Author )
    {
        QHBoxLayout *iconsLayout = new QHBoxLayout( this );
        iconsLayout->setMargin( 0 );
        iconsLayout->setSpacing( 0 );
        m_verticalLayout->insertLayout( m_verticalLayout->count() - 1, iconsLayout );
        iconsLayout->addWidget( m_iconsBar );
        iconsLayout->addWidget( m_snBar );
        iconsLayout->addStretch( 0 );

        m_snBar->setIconSize( QSize( 16, 16 ) );
        m_snBar->setContentsMargins( 0, 0, 0, 0 );
        m_snBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    }
    else
    {
        layout()->addWidget( m_iconsBar );
        m_snBar->hide();
    }

    if( !m_person->emailAddress().isEmpty() )
    {
        QAction *email = new QAction( QIcon::fromTheme( QStringLiteral("mail-send") ), i18n("Email contributor"), this );
        email->setToolTip( m_person->emailAddress() );
        email->setData( QString( "mailto:" + m_person->emailAddress() ) );
        m_iconsBar->addAction( email );
    }

    if( !m_person->webAddress().isEmpty() )
    {
        QAction *homepage = new QAction( QIcon::fromTheme( QStringLiteral("internet-services") ), i18n("Visit contributor's homepage"), this );
        homepage->setToolTip( m_person->webAddress() );
        homepage->setData( m_person->webAddress() );
        m_iconsBar->addAction( homepage );
    }

    connect( m_iconsBar, &KToolBar::actionTriggered, this, &OcsPersonItem::launchUrl );
    connect( m_snBar, &KToolBar::actionTriggered, this, &OcsPersonItem::launchUrl );
    m_textLabel->setText( m_aboutText );
}

OcsPersonItem::~OcsPersonItem()
{}

QString
OcsPersonItem::name() const
{
    return m_person->name();
}

void
OcsPersonItem::launchUrl( QAction *action ) //SLOT
{
    QUrl url = QUrl( action->data().toString() );
    KIO::OpenUrlJob *openUrlJob = new KIO::OpenUrlJob(url, QStringLiteral("text/html"), this );
    openUrlJob->setRunExecutables( true );
    openUrlJob->start();
}

void
OcsPersonItem::switchToOcs( Attica::Provider &provider )
{
    if( m_state == Online )
        return;
    m_avatar->setFixedWidth( 56 );
    m_vertLine->show();
    m_initialSpacer->changeSize( 5, 40, QSizePolicy::Fixed, QSizePolicy::Fixed );
    layout()->invalidate();

    if( !m_ocsUsername.isEmpty() )
    {
        Attica::ItemJob< Attica::Person > *personJob;
        if( m_ocsUsername == QStringLiteral( "%%category%%" ) )   //TODO: handle grouping
            return;

        personJob = provider.requestPerson( m_ocsUsername );
        connect( personJob, &Attica::BaseJob::finished, this, &OcsPersonItem::onJobFinished );
        Q_EMIT ocsFetchStarted();
        m_state = Online;
        personJob->start();
    }
}

void
OcsPersonItem::onJobFinished( Attica::BaseJob *job )
{
    Attica::ItemJob< Attica::Person > *personJob = static_cast< Attica::ItemJob< Attica::Person > * >( job );
    Attica::Metadata metadata = personJob->metadata();
    if( metadata.error() == Attica::Metadata::NoError )
    {
        fillOcsData( personJob->result() );
    }
    Q_EMIT ocsFetchResult( metadata.error() );
}

void
OcsPersonItem::fillOcsData( const Attica::Person &ocsPerson )
{
    if( ocsPerson.avatarUrl().isValid() )
    {
        auto job = KIO::storedGet( ocsPerson.avatarUrl(), KIO::NoReload, KIO::HideProgressInfo );
        connect( job, &KIO::StoredTransferJob::result, this, &OcsPersonItem::onAvatarLoadingJobFinished );
    }

    if( !ocsPerson.country().isEmpty() )
    {
        m_aboutText.append( "<br/>" );
        if( !ocsPerson.city().isEmpty() )
            m_aboutText.append( i18nc( "A person's location: City, Country", "%1, %2", ocsPerson.city(), ocsPerson.country() ) );
        else
            m_aboutText.append( ocsPerson.country() );
    }

    if( m_status == Author )
    {
        if( !ocsPerson.extendedAttribute( QStringLiteral("ircchannels") ).isEmpty() )
        {
            QString channelsString = ocsPerson.extendedAttribute( QStringLiteral("ircchannels") );
            //We extract the channel names from the string provided by OCS:
            QRegularExpression channelrx = QRegularExpression( QLatin1String( "#+[\\w\\.\\-\\/!()+]+([\\w\\-\\/!()+]?)" ), QRegularExpression::CaseInsensitiveOption );
            QStringList channels;
            QRegularExpressionMatch rmatch = channelrx.match( channelsString );
            for (int i = 0; i <= rmatch.lastCapturedIndex(); ++i)
            {
                channels << rmatch.captured(i);
            }

            m_aboutText.append( "<br/>" + i18n("IRC channels: ") );
            QString link;
            for( const QString &channel : channels )
            {
                const QString channelName = QString( channel ).remove( QLatin1Char( '#' ) );
                link = QStringLiteral( "irc://irc.freenode.org/%1" ).arg( channelName );
                m_aboutText.append( QStringLiteral( "<a href=\"%1\">%2</a>" ).arg( link, channel ) + "  " );
            }
        }
        if( !ocsPerson.extendedAttribute( QStringLiteral("favouritemusic") ).isEmpty() )
        {
            QStringList artists = ocsPerson.extendedAttribute( QStringLiteral("favouritemusic") ).split( QStringLiteral(", ") );
            //TODO: make them clickable
            m_aboutText.append( "<br/>" + i18n( "Favorite music: " ) + artists.join( QStringLiteral(", ") ) );
        }
    }

    QAction *visitProfile = new QAction( QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, 
            "amarok/images/opendesktop-22.png" ) ) ), i18n( "Visit %1's openDesktop.org profile", ocsPerson.firstName() ), this );

    visitProfile->setToolTip( i18n( "Visit %1's profile on openDesktop.org", ocsPerson.firstName() ) );

    visitProfile->setData( ocsPerson.extendedAttribute( QStringLiteral("profilepage") ) );
    m_iconsBar->addAction( visitProfile );

    if( m_status == Author )
    {
        QVector< QPair< QString, QString > > ocsHomepages;
        ocsHomepages.append( QPair< QString, QString >( ocsPerson.extendedAttribute( QStringLiteral("homepagetype") ), ocsPerson.homepage() ) );

        debug() << "USER HOMEPAGE DATA STARTS HERE";
        debug() << ocsHomepages.last().first << " :: " << ocsHomepages.last().second;

        for( int i = 2; i <= 10; i++ )  //OCS supports 10 total homepages as of 2/oct/2009
        {
            QString type = ocsPerson.extendedAttribute( QStringLiteral( "homepagetype%1" ).arg( i ) );
            ocsHomepages.append( QPair< QString, QString >( ( type == QLatin1String("&nbsp;") ) ? QLatin1String("") : type,
                                                            ocsPerson.extendedAttribute( QStringLiteral( "homepage%1" ).arg( i ) ) ) );
            debug() << ocsHomepages.last().first << " :: " << ocsHomepages.last().second;
        }

        bool fillHomepageFromOcs = m_person->webAddress().isEmpty();    //We check if the person already has a homepage in KAboutPerson.

        for( QVector< QPair< QString, QString > >::const_iterator entry = ocsHomepages.constBegin();
             entry != ocsHomepages.constEnd(); ++entry )
        {
            QString type = (*entry).first;
            QString url = (*entry).second;
            QIcon icon;
            QString text;

            if( type == QLatin1String("Blog") )
            {
                icon = QIcon::fromTheme( QStringLiteral("kblogger") );
                text = i18n( "Visit contributor's blog" );
            }
            else if( type == QLatin1String("delicious") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-delicious.png" ) ) );
                text = i18n( "Visit contributor's del.icio.us profile" );
            }
            else if( type == QLatin1String("Digg") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-digg.png" ) ) );
                text = i18n( "Visit contributor's Digg profile" );
            }
            else if( type == QLatin1String("Facebook") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-facebook.png" ) ) );
                text = i18n( "Visit contributor's Facebook profile" );
            }
            else if( type == QLatin1String("Homepage") || type == QLatin1String("other") || ( type.isEmpty() && !url.isEmpty() ) )
            {
                if( fillHomepageFromOcs )
                {
                    QAction *homepage = new QAction( QIcon::fromTheme( QStringLiteral("internet-services") ), i18n("Visit contributor's homepage"), this );
                    homepage->setToolTip( url );
                    homepage->setData( url );
                    m_iconsBar->addAction( homepage );
                    fillHomepageFromOcs = false;
                    continue;
                }
                if( type == QLatin1String("other") && url.contains( QLatin1String("last.fm/") ) )     //HACK: assign a last.fm icon if the URL contains last.fm
                {
                    icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-lastfm.png" ) ) );
                    text = i18n( "Visit contributor's Last.fm profile" );
                }
                else
                    continue;
            }
            else if( type == QLatin1String("LinkedIn") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-linkedin.png" ) ) );
                text = i18n( "Visit contributor's LinkedIn profile" );
            }
            else if( type == QLatin1String("MySpace") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-myspace.png" ) ) );
                text = i18n( "Visit contributor's MySpace homepage" );
            }
            else if( type == QLatin1String("Reddit") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-reddit.png" ) ) );
                text = i18n( "Visit contributor's Reddit profile" );
            }
            else if( type == QLatin1String("YouTube") )
            {
                icon = QIcon( "dragonplayer" ); //FIXME: icon
                text = i18n( "Visit contributor's YouTube profile" );
            }
            else if( type == QLatin1String("Twitter") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-twitter.png" ) ) );
                text = i18n( "Visit contributor's Twitter feed" );
            }
            else if( type == QLatin1String("Wikipedia") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-wikipedia.png" ) ) );
                text = i18n( "Visit contributor's Wikipedia profile" );
            }
            else if( type == QLatin1String("Xing") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-xing.png" ) ) );
                text = i18n( "Visit contributor's Xing profile" );
            }
            else if( type == QLatin1String("identi.ca") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-identica.png" ) ) );
                text = i18n( "Visit contributor's identi.ca feed" );
            }
            else if( type == QLatin1String("libre.fm") )
            {
                icon = QIcon( "juk" );  //FIXME: icon
                text = i18n( "Visit contributor's libre.fm profile" );
            }
            else if( type == QLatin1String("StackOverflow") )
            {
                icon = QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-stackoverflow.png" ) ) );
                text = i18n( "Visit contributor's StackOverflow profile" );
            }
            else
                break;
            QAction *action = new QAction( icon, text, this );
            action->setToolTip( url );
            action->setData( url );
            m_snBar->addAction( action );
        }

        debug() << "END USER HOMEPAGE DATA";
    }

    m_textLabel->setText( m_aboutText );
}

void
OcsPersonItem::onAvatarLoadingJobFinished( KJob *job )
{
    auto storedJob = qobject_cast< KIO::StoredTransferJob * >( job );
    if( storedJob->error() )
    {
        debug() << "failed to download the avatar of" << m_ocsUsername << "error:" << storedJob->errorString();
        return;
    }

    QPixmap pic;
    if ( !pic.loadFromData( storedJob->data() ) )
    {
        debug() << "failed to load the avatar of" << m_ocsUsername;
        return;
    }

    m_avatar->setFixedSize( 56, 56 );
    m_avatar->setFrameShape( QFrame::StyledPanel ); //this is a FramedLabel, otherwise oxygen wouldn't paint the frame
    m_avatar->setPixmap( pic );
    m_avatar->setAlignment( Qt::AlignCenter );
}

