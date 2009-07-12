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

#ifndef MEDIA_DEVICES_APPLET_H
#define MEDIA_DEVICES_APPLET_H

#include <KIcon>

#include <QList>
#include <QMap>
#include <QStringList>

#include <context/Applet.h>
#include <context/Svg.h>

namespace Plasma {
    class IconWidget;
    class Label;
}


class QSizeF;
class QGraphicsLinearLayout;

// A convenience class to store and send connection information
class DeviceInfo : public QObject
{
    Q_OBJECT
    public:
        DeviceInfo();
        virtual ~DeviceInfo();

        virtual QGraphicsLinearLayout* layout();

    public slots:
        virtual void connectClicked();
        virtual void disconnectClicked();

    protected:
        bool m_connected;


        QGraphicsLinearLayout *m_layout;


};


class IpodInfo : public DeviceInfo
{
    Q_OBJECT
    public:
        IpodInfo( QGraphicsWidget *applet, const QString &mountpoint, const QString &udi );
        virtual ~IpodInfo();

        virtual QGraphicsLinearLayout* layout();

    signals:
        void readyToConnect( const QString &mountpoint, const QString &udi );
        void readyToDisconnect( const QString &udi );

    public slots:
        virtual void connectClicked();
        virtual void disconnectClicked();

    private:
        QGraphicsWidget *m_applet;
        QString m_mountpoint;
        QString m_udi;
};

class MtpInfo : public DeviceInfo
{
    Q_OBJECT
    public:
        MtpInfo( QGraphicsWidget *applet, const QString &serial, const QString &udi );
        virtual ~MtpInfo();

        virtual QGraphicsLinearLayout* layout();

    signals:
        void readyToConnect( const QString &serial, const QString &udi );
        void readyToDisconnect( const QString &udi );

    public slots:
        virtual void connectClicked();
        virtual void disconnectClicked();

    private:
        QGraphicsWidget *m_applet;
        QString m_serial;
        QString m_udi;
};


// Define our plasma Applet
class MediaDevicesApplet : public Context::Applet
{
    Q_OBJECT
    public:
        // Basic Create/Destroy
        MediaDevicesApplet(QObject *parent, const QVariantList &args);
        ~MediaDevicesApplet();

        // The paintInterface procedure paints the applet to screen
        void paintInterface(QPainter *painter,
        const QStyleOptionGraphicsItem *option,
        const QRect& contentsRect);
        void init();

//        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

    private:

        void redraw();

        Plasma::IconWidget *m_icon;
        Plasma::IconWidget *m_connect;
        Plasma::IconWidget *m_disconnect;
        QGraphicsLinearLayout *m_layout;

        //QList<IpodInfo> m_ipodInfoList;
        QMap<QString, DeviceInfo*> m_infoMap; // maps udi to DeviceInfo
        //QList<QGraphicsLinearLayout> m_layoutList;
        QStringList m_udiList;

    private slots:
        void ipodDetected( const QString &mountPoint, const QString &udi );
        void mtpDetected( const QString &serial, const QString &udi );
        void deviceRemoved( const QString &udi );

};

K_EXPORT_AMAROK_APPLET( mediadevices, MediaDevicesApplet )

#endif
