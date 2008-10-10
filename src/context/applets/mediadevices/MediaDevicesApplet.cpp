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

 // NOTE: code for % free space bar referenced from:
 // kdebase/workspace/plasma/applets/kickoff
 // kdelibs/kio/kfile/kpropertiesdialog.cpp

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

DeviceInfo::DeviceInfo()
    : QObject()
{
    m_connected = false;
}

DeviceInfo::~DeviceInfo()
{
}

QGraphicsLinearLayout*
DeviceInfo::layout()
{
    return m_layout;
}

void
DeviceInfo::connectClicked()
{
}

void
DeviceInfo::disconnectClicked()
{
}

IpodInfo::IpodInfo( QGraphicsWidget *applet, const QString &mountpoint, const QString &udi )
    : DeviceInfo(),
    m_mountpoint( mountpoint ),
    m_udi( udi )
{
    debug() << "Creating layout";

    m_layout = new QGraphicsLinearLayout( Qt::Horizontal );
    m_layout->setSpacing( 0 );

    // set up icons

    // get path where we pull svg's from

    debug() << "Getting svg path";

    QString svgPath = KStandardDirs::locate( "data", "amarok/images/pud_items.svg" );

    // iPod (device) icon

    debug() << "Icon stuff";

    Plasma::Icon *ipodIcon = new Plasma::Icon( applet );
    ipodIcon->setSvg( svgPath, "device" );

    // NOTE: at some point connect/disconnect icon should be merged somehow

    // Connect Icon

    Plasma::Icon *connectIcon = new Plasma::Icon( applet );
    connectIcon->setSvg( svgPath, "append" );

    QAction *connectAction = new QAction( this );
    connect( connectAction, SIGNAL( activated() ),
             SLOT( connectClicked() ) );

    connectIcon->setAction( connectAction );

    // Disconnect Icon

    Plasma::Icon *disconnectIcon = new Plasma::Icon( applet );
    disconnectIcon->setSvg( svgPath, "delete" );

    QAction *disconnectAction = new QAction( this );
    connect( disconnectAction, SIGNAL( activated() ),
             SLOT( disconnectClicked() ) );

    disconnectIcon->setAction( disconnectAction );

    // Text label

    debug() << "Label";

    Plasma::Label *deviceLabel = new Plasma::Label( applet );
    deviceLabel->setText( mountpoint );

    // set icons sizes

    debug() << "Set sizes";

    QSizeF iconSize = ipodIcon->sizeFromIconSize(32);
    
    ipodIcon->setMinimumSize( iconSize );
    ipodIcon->setMaximumSize( iconSize );

    connectIcon->setMinimumSize( iconSize );
    connectIcon->setMaximumSize( iconSize );

    disconnectIcon->setMinimumSize( iconSize );
    disconnectIcon->setMaximumSize( iconSize );

//    deviceLabel->setMinimumSize( iconSize );
//    deviceLabel->setMaximumSize( iconSize );

    // put icons into layout

    debug() << "Add icons to layout";

    m_layout->addItem( ipodIcon );
    m_layout->addItem( connectIcon );
    m_layout->addItem( disconnectIcon );
    m_layout->addItem( deviceLabel );

    // add the new layout to the main layout
    
    connect( this, SIGNAL( readyToConnect( const QString &, const QString &) ),
             MediaDeviceMonitor::instance(), SLOT( connectIpod( const QString &, const QString & ) ) );
    connect( this, SIGNAL( readyToDisconnect( const QString & ) ),
             MediaDeviceMonitor::instance(), SLOT( disconnectIpod( const QString & ) ) );
}


IpodInfo::~IpodInfo()
{
}

void
IpodInfo::connectClicked()
{
    emit readyToConnect( m_mountpoint, m_udi );
}

void
IpodInfo::disconnectClicked()
{
    emit readyToDisconnect( m_udi );
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

    // if already in list, do not add twice to applet

    if( m_udiList.contains( udi ) )
            return;
    else
        m_udiList << udi;

    // set up info class

    IpodInfo *ipodInfo = new IpodInfo( this, mountPoint, udi );
    //m_ipodInfoList.insert( m_ipodInfoList.size(), *ipodInfo );

    // add the new layout to the main layout

    debug() << "Add to main layout";

    m_layout->addItem( ipodInfo->layout() );

    debug() << "Successfully added ipodLayout to layout";
}
 
#include "MediaDevicesApplet.moc"
