/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#ifndef AMAROK_ALBUMSDELEGATE_H
#define AMAROK_ALBUMSDELEGATE_H

#include <context/Svg.h>

#include <QtGui/QAbstractItemDelegate>

#include <KIcon>

namespace AlbumRoles
{
    enum AlbumRolId
    {
        AlbumName = Qt::UserRole + 1,
        TrackCount,
        AlbumCover,
        TrackName,
    };
}

class AlbumsDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    AlbumsDelegate( QObject *parent = 0 );
    ~AlbumsDelegate();

    //Reimplemented
    virtual void paint( QPainter *painter,const QStyleOptionViewItem& option,const QModelIndex& index ) const;
public slots:
    void highlightRow( const QModelIndex &index );
protected:

    virtual QSize sizeHint( const QStyleOptionViewItem& option , const QModelIndex& index ) const;

private:
    Context::Svg *m_decorations;
    int m_coverWidth;
    QModelIndex m_highlightedRow;

    KIcon m_coverIcon;
    KIcon m_trackIcon;    
};

#endif
