/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPERACTION_H
#define POPUPDROPPERACTION_H

#include <QAction>
#include <QString>

class QSvgRenderer;
class PopupDropperActionPrivate;

// A QAction with support for SVGs

class PopupDropperAction : public QAction
{
    Q_OBJECT

    Q_PROPERTY( QSvgRenderer* renderer READ renderer WRITE setRenderer )
    Q_PROPERTY( QString elementId READ elementId WRITE setElementId )

public:
    PopupDropperAction( QObject *parent );
    PopupDropperAction( const QString &file, const QString &text, QObject *parent );
    PopupDropperAction( const QByteArray &contents, const QString &text, QObject *parent );
    PopupDropperAction( QSvgRenderer* renderer, const QString &text, QObject *parent );
    PopupDropperAction( QSvgRenderer* renderer, const QString &elementId, const QString &text, QObject *parent );
    
    ~PopupDropperAction();

    QSvgRenderer* renderer() const;
    void setRenderer( QSvgRenderer *renderer );
    QString elementId() const;
    void setElementId( const QString &id );

private:
    friend class PopupDropperActionPrivate;
    PopupDropperActionPrivate* const d;
};

#endif
