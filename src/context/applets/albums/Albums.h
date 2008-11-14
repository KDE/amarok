/****************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>                  *
 *                        (C) 2008 William Viana Soares <vianasw@gmail.com> *
 ****************************************************************************/

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

#include "AlbumsView.h"

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <meta/Meta.h>

#include <KDialog>

#include <QList>
#include <QAction>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QStandardItemModel>

class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;

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

private:
    void prepareElements();
    QList<QAction*> contextualActions();

    QHBoxLayout* m_configLayout;
    int m_width;
    int m_height;
    const qreal m_albumWidth;

    qreal m_aspectRatio;

    Meta::AlbumList m_albums;
    int m_albumCount;

    int m_maxTextWidth;
    QStandardItemModel *m_model;

    AlbumsView *m_albumsView;

    QGraphicsSimpleTextItem *m_headerText;
    QVariantList m_albumsTracks;
};

K_EXPORT_AMAROK_APPLET( albums, Albums )

#endif
