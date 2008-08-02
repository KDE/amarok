/***************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>                 *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ALBUMS_APPLET_H
#define ALBUMS_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/Svg.h>

#include <KDialog>

#include <QList>
#include <QAction>

class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;

/**
 * An album text item that responds to being clicked
 */
class AlbumTextItem : public QObject, public QGraphicsSimpleTextItem
{
    Q_OBJECT
public:
    explicit AlbumTextItem( QGraphicsItem *parent = 0 );

protected:
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );

signals:
    void clicked( const QString &text );
};

class Albums : public Context::Applet
{
    Q_OBJECT
public:
    Albums( QObject* parent, const QVariantList& args );
    ~Albums();

    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints);
    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );
    void showConfigurationInterface();


private slots:
    void configAccepted();
    void connectSource( const QString &source );
    void enqueueAlbum( const QString &name );

private:
    void prepareElements();
    QList<QAction*> contextualActions();

    QHBoxLayout* m_configLayout;
    int m_width;
    const qreal m_albumWidth;

    qreal m_aspectRatio;

    Context::Svg* m_theme;

    QGraphicsSimpleTextItem*        m_artistLabel;
    QList<AlbumTextItem*>           m_albumLabels;
    QList<QGraphicsSimpleTextItem*> m_albumTracks;
    QList<QGraphicsPixmapItem*>     m_albumCovers;

    int m_albumCount;
    QVariantList m_names;
    QVariantList m_trackCounts;
    QVariantList m_covers;

    int m_maxTextWidth;
};

K_EXPORT_AMAROK_APPLET( albums, Albums )

#endif
