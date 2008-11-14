/***************************************************************************
 * copyright            : (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>   *
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
    ~LastFmEvents();

    void init();

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints);

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

    void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& contentsRect);
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private slots:
    void connectSource( const QString &source );
private:

    qreal m_aspectRatio;
    qreal m_width;

    Context::Svg* m_theme;

    // items that go inside the svg
    QList< QGraphicsSimpleTextItem* > m_titles;
    QList< QGraphicsSimpleTextItem* > m_dates;
    QList< QGraphicsSimpleTextItem* > m_cities;
};

K_EXPORT_AMAROK_APPLET( lastfmevents, LastFmEvents )

#endif
