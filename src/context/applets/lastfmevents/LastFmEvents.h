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

#ifndef LAST_FM_EVENTS_H
#define LAST_FM_EVENTS_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/widgets/TextWidget.h>
#include <context/Svg.h>
#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QCheckBox>

#include <KDialog>

class LastFmEvents : public Context::Applet
{
    Q_OBJECT
public:
    LastFmEvents( QObject* parent, const QVariantList& args );

    void constraintsUpdated();

    void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& contentsRect);
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
    void showConfigurationInterface();

private slots:
    void configAccepted();

private:
    QString truncateTextToFit( QString text, const QFont& font, const QRectF& bounds );
    QFont shrinkTextSizeToFit( const QString& text, const QRectF& bounds );

    void resizeApplet( qreal oldWidth, qreal aspectRatio );

    KDialog* m_config;
    QVBoxLayout* m_configLayout;
    QCheckBox* m_friendBox;
    QCheckBox* m_sysBox;
    QCheckBox* m_userBox;

    qreal m_width;

    Context::Svg* m_theme;

    // items that go inside the svg
    QList< QGraphicsSimpleTextItem* > m_titles;
    QList< QGraphicsSimpleTextItem* > m_dates;
    QList< QGraphicsSimpleTextItem* > m_cities;

    bool m_friendEnabled;
    bool m_sysEnabled;
    bool m_userEnabled;
};

K_EXPORT_AMAROK_APPLET( lastfmevents, LastFmEvents )

#endif
