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

#ifndef MEDIA_DEVICES_APPLET_H
#define MEDIA_DEVICES_APPLET_H

#include <KIcon>

#include <QList>

#include <context/Applet.h>
#include <context/Svg.h>

namespace Plasma {
    class Icon;
    class Label;
}


class QSizeF;
class QGraphicsLinearLayout;

// A convenience class to store and send connection information
class IpodInfo : public QObject
{
    Q_OBJECT
    public:
        IpodInfo( QObject *parent, const QString &mountpoint, const QString &udi );
        ~IpodInfo();

    signals:
        void readyToConnect( const QString &mountpoint, const QString &udi );

    public slots:
        void connectClicked();

    private:
        QString m_mountpoint;
        QString m_udi;
};

//class QList<IpodInfo>;


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

        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

    private:
        Plasma::Icon *m_icon;
        Plasma::Icon *m_connect;
        Plasma::Icon *m_disconnect;
        QGraphicsLinearLayout *m_layout;

        QList<IpodInfo> m_ipodInfoList;

    private slots:
        void ipodDetected( const QString &mountPoint, const QString &udi );

};

K_EXPORT_AMAROK_APPLET( mediadevices, MediaDevicesApplet )

#endif
