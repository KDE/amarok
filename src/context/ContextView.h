/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
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

/*
  Significant parts of this code is inspired and/or copied from KDE Plasma sources,
  available at kdebase/workspace/libs/plasma
*/

#ifndef AMAROK_CONTEXT_VIEW_H
#define AMAROK_CONTEXT_VIEW_H


#include "amarok_export.h"

#include <QQuickWidget>

class ContextUrlRunner;
class FontFilter;
class QPalette;
class QQmlPropertyMap;
class QScreen;

namespace Context
{

class AppletLoader;
class AppletModel;
class AppletProxyModel;

class AMAROK_EXPORT ContextView : public QQuickWidget
{
    Q_OBJECT

    // Properties copied from KF5::Plasma::Units
    Q_PROPERTY( int smallSpacing READ smallSpacing NOTIFY spacingChanged )
    Q_PROPERTY( int largeSpacing READ largeSpacing NOTIFY spacingChanged )
    Q_PROPERTY( QQmlPropertyMap *iconSizes READ iconSizes CONSTANT )
    Q_PROPERTY( qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged )

public:
     ContextView( QWidget *parent = Q_NULLPTR );
    ~ContextView();

    /**
     * Singleton pattern accessor. May return 0 if the view was not yet constructed.
     */
    static ContextView *self() { return s_self; }

    /**
     * Get the plugin names, in order, of the applets currently in the contextView.
    */
    QStringList currentApplets() const;

    /**
     * Get the user visible applet names, in order, of the applets currently in the contextView.
    */
    QStringList currentAppletNames() const;

    /**
     * Get the Context::AppletModel instance in use.
     * It can be used to show, hide enable or disable applets among other things.
     */
    AppletProxyModel *appletModel() const { return m_proxyModel; }

    Q_INVOKABLE void runLink( const QUrl &link ) const;
    Q_INVOKABLE void debug( const QString &error ) const;
    Q_INVOKABLE void warning( const QString &error ) const;
    Q_INVOKABLE void error( const QString &error ) const;

    int smallSpacing() const { return m_smallSpacing; }
    int largeSpacing() const { return m_largeSpacing; }
    QQmlPropertyMap* iconSizes() const { return m_iconSizes; }
    qreal devicePixelRatio() const { return m_devicePixelRatio; }

private slots:
    void slotStatusChanged( QQuickWidget::Status status );
    void updateSpacing();
    void updateDevicePixelRatio( QScreen *screen );
    void iconLoaderSettingsChanged();
    void updatePalette( const QPalette &palette );

signals:
    void spacingChanged();
    void iconSizesChanged();
    void devicePixelRatioChanged();

private:
    // copied from KF5::Plasma::Units
    int devicePixelIconSize( int size ) const;

    static ContextView *s_self;

    ContextUrlRunner *m_urlRunner;
    AppletLoader *m_loader;
    AppletModel *m_appletModel;
    AppletProxyModel *m_proxyModel;
    FontFilter *m_fontFilter;

    int m_smallSpacing;
    int m_largeSpacing;
    QQmlPropertyMap *m_iconSizes;
    qreal m_devicePixelRatio;
};

} // Context namespace

#endif
