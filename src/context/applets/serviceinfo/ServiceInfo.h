/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SERVICE_INFO_APPLET_H
#define SERVICE_INFO_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/Svg.h>

#include "meta/XSPFPlaylist.h"
#include "meta/Playlist.h"

#include <plasma/framesvg.h>
#include <plasma/widgets/webview.h>
 
#include <KDialog>

#include <QGraphicsProxyWidget>
#include <qwebview.h>


class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;
class QSpinBox;
class QCheckBox;

class ServiceInfo : public Context::Applet, public Meta::PlaylistObserver
{
    Q_OBJECT
public:
    ServiceInfo( QObject* parent, const QVariantList& args );
    virtual ~ServiceInfo();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

    virtual void trackListChanged( Meta::Playlist* playlist );

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );
    void showConfigurationInterface();

private slots:
    void configAccepted();
    void linkClicked( const QUrl & url );

private:
    void resize( qreal newWidth, qreal aspectRatio );

    KDialog* m_config;
    QHBoxLayout* m_configLayout;
    QSpinBox* m_spinWidth;
    int m_width;

    qreal m_aspectRatio;

    Context::Svg* m_header;
    Plasma::FrameSvg *m_theme;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_serviceName;
    //QGraphicsProxyWidget* m_serviceMainInfo;

    //QWebView * m_webView;

    Plasma::WebView * m_webView;

    bool m_initialized;

    Meta::XSPFPlaylist * m_currentPlaylist;


};

K_EXPORT_AMAROK_APPLET( serviceinfo, ServiceInfo )

#endif
