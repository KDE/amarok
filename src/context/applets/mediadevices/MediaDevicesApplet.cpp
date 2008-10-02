/*****************************************************************************
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarauzni@gmail.com>          *
*
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MediaDevicesApplet.h"

#include "MediaDeviceMonitor.h"

#include <QAction>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
 
#include <plasma/svg.h>
#include <plasma/theme.h>
#include <plasma/widgets/icon.h>
#include <plasma/widgets/label.h>

#include "Debug.h"

#include <KStandardDirs>

IpodInfo::IpodInfo( QObject *parent, const QString &mountpoint, const QString &udi )
    : QObject( parent ),
    m_mountpoint( mountpoint ),
    m_udi( udi )
{
    connect( this, SIGNAL( readyToConnect( const QString &, const QString &) ),
             MediaDeviceMonitor::instance(), SLOT( connectIpod( const QString &, const QString & ) ) );
}


IpodInfo::~IpodInfo()
{
}

void
IpodInfo::connectClicked()
{
    emit readyToConnect( m_mountpoint, m_udi );
}
 
MediaDevicesApplet::MediaDevicesApplet(QObject *parent, const QVariantList &args)
    : Context::Applet(parent, args),
    m_icon(0),
    m_connect(0),
    m_disconnect(0)
{
    // this will get us the standard applet background
    setBackgroundHints(DefaultBackground);
}
 
 
MediaDevicesApplet::~MediaDevicesApplet()
{
    if (hasFailedToLaunch()) {
        // Do some cleanup here

        //delete m_ipodInfoList;
    } else {
        // Save settings
    }
}
 
void MediaDevicesApplet::init()
{
    // layout and ui

    // set initially blank layout

    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    setLayout( m_layout );
    m_layout->setSpacing( 0 );

    // add items to layout
/*
    m_layout->addItem( m_icon );
    m_layout->addItem( m_connect );
    m_layout->addItem( m_disconnect );
*/

    // init list that stores info

    // connect to the monitor

    connect( MediaDeviceMonitor::instance(), SIGNAL( ipodDetected( const QString &, const QString & ) ),
    SLOT( ipodDetected( const QString &, const QString & ) ) );

    MediaDeviceMonitor::instance()->checkDevicesForIpod();
} 
 
 
void
MediaDevicesApplet::paintInterface(QPainter *p,
        const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    p->setRenderHint(QPainter::Antialiasing);
 
    // Now we draw the applet, starting with our svg

    // We place the icon and text
//    p->drawPixmap(7, 0, m_icon.pixmap((int)contentsRect.width(),(int)contentsRect.width()-14));
    /*
    p->save();
    p->setPen(Qt::white);
    p->drawText(contentsRect,
                Qt::AlignBottom | Qt::AlignHCenter,
                "Hello Plasmoid!");
    
    p->restore();
    */
}

QSizeF 
MediaDevicesApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

            if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
            return QSizeF( constraint.width(), 150 );
//         return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );

    return constraint;
}

void
MediaDevicesApplet::ipodDetected( const QString &mountPoint, const QString &udi )
{

    DEBUG_BLOCK

    // set up info class

    IpodInfo *ipodInfo = new IpodInfo( this, mountPoint, udi );
    //m_ipodInfoList.insert( m_ipodInfoList.size(), *ipodInfo );

    // set up new layout

    debug() << "Creating layout";

    QGraphicsLinearLayout *ipodLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    ipodLayout->setSpacing( 0 );

    // set up icons

    // get path where we pull svg's from

    debug() << "Getting svg path";

    QString svgPath = KStandardDirs::locate( "data", "amarok/images/pud_items.svg" );

    // iPod (device) icon

    debug() << "Icon stuff";

    Plasma::Icon *ipodIcon = new Plasma::Icon( this );
    ipodIcon->setSvg( svgPath, "device" );

    // Connect Icon

    Plasma::Icon *connectIcon = new Plasma::Icon( this );
    connectIcon->setSvg( svgPath, "append" );

    QAction *connectAction = new QAction( this );
    connect( connectAction, SIGNAL( activated() ),
             ipodInfo, SLOT( connectClicked() ) );

    connectIcon->setAction( connectAction );

    debug() << "Label";

    // Text label

    Plasma::Label *deviceLabel = new Plasma::Label( this );
    deviceLabel->setText( mountPoint );

    // set icons sizes

    debug() << "Set sizes";

    QSizeF iconSize = ipodIcon->sizeFromIconSize(32);
    
    ipodIcon->setMinimumSize( iconSize );
    ipodIcon->setMaximumSize( iconSize );

    connectIcon->setMinimumSize( iconSize );
    connectIcon->setMaximumSize( iconSize );

//    deviceLabel->setMinimumSize( iconSize );
//    deviceLabel->setMaximumSize( iconSize );

    // put icons into layout

    debug() << "Add icons to layout";

    ipodLayout->addItem( ipodIcon );
    ipodLayout->addItem( connectIcon );
    ipodLayout->addItem( deviceLabel );

    // add the new layout to the main layout

    debug() << "Add to main layout";

    m_layout->addItem( ipodLayout );

    debug() << "Successfully added ipodLayout to layout";
}
 
#include "MediaDevicesApplet.moc"
