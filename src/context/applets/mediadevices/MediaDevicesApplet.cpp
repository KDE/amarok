/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MediaDevicesApplet.h"

#include "MediaDeviceMonitor.h"

#include <QAction>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QSizeF>
 
#include <plasma/svg.h>
#include <plasma/theme.h>
#include <plasma/widgets/iconwidget.h>
#include <plasma/widgets/label.h>

#include "Debug.h"

#include <KStandardDirs>

DeviceInfo::DeviceInfo()
    : QObject()
{
    DEBUG_BLOCK
    m_connected = false;
}

DeviceInfo::~DeviceInfo()
{
}

QGraphicsLinearLayout*
DeviceInfo::layout()
{
    DEBUG_BLOCK
    return m_layout;
}

void
DeviceInfo::connectClicked()
{
    DEBUG_BLOCK
}

void
DeviceInfo::disconnectClicked()
{
    DEBUG_BLOCK
}

IpodInfo::IpodInfo( QGraphicsWidget *applet, const QString &mountpoint, const QString &udi )
    : DeviceInfo(),
    m_applet( applet ),
    m_mountpoint( mountpoint ),
    m_udi( udi )
{
    DEBUG_BLOCK

    connect( this, SIGNAL( readyToConnect( const QString &, const QString &) ),
             MediaDeviceMonitor::instance(), SLOT( connectIpod( const QString &, const QString & ) ) );
    connect( this, SIGNAL( readyToDisconnect( const QString & ) ),
             MediaDeviceMonitor::instance(), SLOT( disconnectIpod( const QString & ) ) );
}


IpodInfo::~IpodInfo()
{
}

QGraphicsLinearLayout*
IpodInfo::layout()
{
    DEBUG_BLOCK

    m_layout = new QGraphicsLinearLayout( Qt::Horizontal );
    m_layout->setSpacing( 0 );

    // set up icons

    // get path where we pull svg's from
    QString svgPath = KStandardDirs::locate( "data", "amarok/images/pud_items.svg" );

    // iPod (device) icon
    Plasma::IconWidget *ipodIcon = new Plasma::IconWidget( m_applet );
    ipodIcon->setSvg( svgPath, "device" );

    // NOTE: at some point connect/disconnect icon should be merged somehow

    // Connect Icon
    Plasma::IconWidget *connectIcon = new Plasma::IconWidget( m_applet );
    connectIcon->setSvg( svgPath, "append" );

    QAction *connectAction = new QAction( this );
    connect( connectAction, SIGNAL( activated() ),
             SLOT( connectClicked() ) );

    connectIcon->setAction( connectAction );

    // Disconnect Icon

    Plasma::IconWidget *disconnectIcon = new Plasma::IconWidget( m_applet );
    disconnectIcon->setSvg( svgPath, "delete" );

    QAction *disconnectAction = new QAction( this );
    connect( disconnectAction, SIGNAL( activated() ),
             SLOT( disconnectClicked() ) );

    disconnectIcon->setAction( disconnectAction );

    // Text label

    debug() << "Label";

    Plasma::Label *deviceLabel = new Plasma::Label( m_applet );
    deviceLabel->setText( m_mountpoint );

    // set icons sizes

    debug() << "Set sizes";

    QSizeF iconSize = ipodIcon->sizeFromIconSize(32);
    
    ipodIcon->setMinimumSize( iconSize );
    ipodIcon->setMaximumSize( iconSize );

    connectIcon->setMinimumSize( iconSize );
    connectIcon->setMaximumSize( iconSize );

    disconnectIcon->setMinimumSize( iconSize );
    disconnectIcon->setMaximumSize( iconSize );

    // put icons into layout

    debug() << "Add icons to layout";

    m_layout->addItem( ipodIcon );
    m_layout->addItem( connectIcon );
    m_layout->addItem( disconnectIcon );
    m_layout->addItem( deviceLabel );

    return m_layout;
}

void
IpodInfo::connectClicked()
{
    DEBUG_BLOCK
    emit readyToConnect( m_mountpoint, m_udi );
}

void
IpodInfo::disconnectClicked()
{
    DEBUG_BLOCK
    emit readyToDisconnect( m_udi );
}

// Begin MtpInfo

MtpInfo::MtpInfo( QGraphicsWidget *applet, const QString &serial, const QString &udi )
    : DeviceInfo(),
    m_applet( applet ),
    m_serial( serial ),
    m_udi( udi )
{
    DEBUG_BLOCK


    connect( this, SIGNAL( readyToConnect( const QString &, const QString &) ),
                     MediaDeviceMonitor::instance(), SLOT( connectMtp( const QString &, const QString & ) ) );
    connect( this, SIGNAL( readyToDisconnect( const QString & ) ),
             MediaDeviceMonitor::instance(), SLOT( disconnectMtp( const QString & ) ) );
}


MtpInfo::~MtpInfo()
{
}

QGraphicsLinearLayout*
MtpInfo::layout()
{
    DEBUG_BLOCK

    debug() << "Creating layout";

    m_layout = new QGraphicsLinearLayout( Qt::Horizontal );
    m_layout->setSpacing( 0 );

    // set up icons

    // get path where we pull svg's from

    debug() << "Getting svg path";

    QString svgPath = KStandardDirs::locate( "data", "amarok/images/pud_items.svg" );

    // MTP (device) icon

    debug() << "Icon stuff";

    Plasma::IconWidget *mtpIcon = new Plasma::IconWidget( m_applet );
    mtpIcon->setSvg( svgPath, "device" );

    // NOTE: at some point connect/disconnect icon should be merged somehow

    // Connect Icon

    Plasma::IconWidget *connectIcon = new Plasma::IconWidget( m_applet );
    connectIcon->setSvg( svgPath, "append" );

    QAction *connectAction = new QAction( this );
    connect( connectAction, SIGNAL( activated() ),
             SLOT( connectClicked() ) );

    connectIcon->setAction( connectAction );

    // Disconnect Icon

    Plasma::IconWidget *disconnectIcon = new Plasma::IconWidget( m_applet );
    disconnectIcon->setSvg( svgPath, "delete" );

    QAction *disconnectAction = new QAction( this );
    connect( disconnectAction, SIGNAL( activated() ),
             SLOT( disconnectClicked() ) );

    disconnectIcon->setAction( disconnectAction );

    // Text label

    debug() << "Label";

    Plasma::Label *deviceLabel = new Plasma::Label( m_applet );
    deviceLabel->setText( m_serial );

    // set icons sizes

    debug() << "Set sizes";

    QSizeF iconSize = mtpIcon->sizeFromIconSize(32);

    mtpIcon->setMinimumSize( iconSize );
    mtpIcon->setMaximumSize( iconSize );

    connectIcon->setMinimumSize( iconSize );
    connectIcon->setMaximumSize( iconSize );

    disconnectIcon->setMinimumSize( iconSize );
    disconnectIcon->setMaximumSize( iconSize );

    // put icons into layout

    debug() << "Add icons to layout";

    m_layout->addItem( mtpIcon );
    m_layout->addItem( connectIcon );
    m_layout->addItem( disconnectIcon );
    m_layout->addItem( deviceLabel );

    return m_layout;
}

void
MtpInfo::connectClicked()
{
    DEBUG_BLOCK
    emit readyToConnect( m_serial, m_udi );
}

void
MtpInfo::disconnectClicked()
{
    DEBUG_BLOCK
    emit readyToDisconnect( m_udi );
}

// Media Devices Applet
 
MediaDevicesApplet::MediaDevicesApplet(QObject *parent, const QVariantList &args)
    : Context::Applet(parent, args),
    m_icon(0),
    m_connect(0),
    m_disconnect(0),
    m_infoMap()
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
    resize( size().width(), 150 );
    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    setLayout( m_layout );
    m_layout->setSpacing( 0 );

    // connect to the monitor

    connect( MediaDeviceMonitor::instance(), SIGNAL( ipodDetected( const QString &, const QString & ) ),
    SLOT( ipodDetected( const QString &, const QString & ) ) );
    connect( MediaDeviceMonitor::instance(), SIGNAL( mtpDetected( const QString &, const QString & ) ),
             SLOT( mtpDetected( const QString &, const QString & ) ) );
    connect( MediaDeviceMonitor::instance(), SIGNAL( deviceRemoved( const QString & ) ), SLOT( deviceRemoved( const QString & ) ) );
    // check for devices, to initialize display of connected devices

    MediaDeviceMonitor::instance()->checkDevicesForIpod();
    MediaDeviceMonitor::instance()->checkDevicesForMtp();
} 
 
 
void
MediaDevicesApplet::paintInterface(QPainter *p,
        const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
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
/*
QSizeF 
MediaDevicesApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

            if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
            return QSizeF( constraint.width(), 150 );
//         return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );

    return constraint;
}*/

void
MediaDevicesApplet::ipodDetected( const QString &mountPoint, const QString &udi )
{

    DEBUG_BLOCK

    debug() << "Ipod with udi: " << udi;

    // if already in list, do not add twice to applet

    if( m_udiList.contains( udi ) )
            return;
    else
        m_udiList << udi;

    // set up info class

    IpodInfo *ipodInfo = new IpodInfo( this, mountPoint, udi );

    m_infoMap.insert( udi, ipodInfo );

    // add the new layout to the main layout

    debug() << "Add to main layout";

    m_layout->addItem( ipodInfo->layout() );

    debug() << "Successfully added ipodLayout to layout";

}

void
MediaDevicesApplet::mtpDetected( const QString &serial, const QString &udi )
{

    DEBUG_BLOCK

    debug() << "Mtp with udi: " << udi;

    // if already in list, do not add twice to applet

    if( m_udiList.contains( udi ) )
        return;
    else
        m_udiList << udi;

    // set up info class

    MtpInfo *mtpInfo = new MtpInfo( this, serial, udi );

    m_infoMap.insert( udi, mtpInfo );

    // add the new layout to the main layout

    debug() << "Add to main layout";

    m_layout->addItem( mtpInfo->layout() );

    debug() << "Successfully added mtpLayout to layout";

}

void
MediaDevicesApplet::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK

    if( m_udiList.count() == 0 )
    {
        debug() << "Device removed, but no devices present, so do nothing";
        return;
    }

    m_udiList.removeOne( udi );
    debug() << "Removing udi: " << udi;
    m_infoMap.remove( udi );

    redraw();
}

// a device has been removed or something, so the applet redraws itself with the leftover devices

void
MediaDevicesApplet::redraw()
{
    DEBUG_BLOCK

    // clear out things in old layout

            

    debug() << "Clearing out old layout";

    foreach( QGraphicsItem *item, childItems() )
    {
        delete item;
    }

    // create new layout

    QGraphicsLinearLayout *newLayout = new QGraphicsLinearLayout( Qt::Vertical );

    if( m_infoMap.size() == 0 )
    {
        //TODO
    }



    // iterate over devices, add their layouts to main layout

    foreach( const QString &udi, m_infoMap.keys() )
    {
        debug() << "Pulling out device";
        DeviceInfo *device = m_infoMap[udi];
        debug() << "Adding to layout";
        newLayout->addItem( device->layout() );
    }

    debug() << "Deleting old layout";

    m_layout = newLayout;

    debug() << "Setting new layout";

    setLayout( m_layout );
    m_layout->setSpacing( 0 );

    update();
}
 
#include "MediaDevicesApplet.moc"

