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
#include <QIcon>
#include <QPen>
#include <QString>

#include "PopupDropper_Export.h"

class QSvgRenderer;
class PopupDropperActionPrivate;

// A QAction with support for SVGs

class POPUPDROPPER_EXPORT PopupDropperAction : public QAction
{
    Q_OBJECT

    Q_PROPERTY( QSvgRenderer* renderer READ renderer WRITE setRenderer )
    Q_PROPERTY( QString elementId READ elementId WRITE setElementId )
    Q_PROPERTY( bool separator READ isSeparator WRITE setSeparator )
    Q_PROPERTY( bool hasSeparatorPen READ hasSeparatorPen )
    Q_PROPERTY( QPen separatorPen READ separatorPen WRITE setSeparatorPen )

public:
    PopupDropperAction( QObject *parent );
    PopupDropperAction( const QString &text, QObject *parent );
    PopupDropperAction( const QString &file, const QString &text, QObject *parent );
    PopupDropperAction( const QString &file, const QIcon &icon, const QString &text, QObject *parent );
    PopupDropperAction( const QIcon &icon, const QString &text, QObject *parent );
    PopupDropperAction( const QByteArray &contents, const QString &text, QObject *parent );
    PopupDropperAction( const QByteArray &contents, const QIcon &icon, const QString &text, QObject *parent );
    PopupDropperAction( QSvgRenderer* renderer, const QString &text, QObject *parent );
    PopupDropperAction( QSvgRenderer* renderer, const QIcon &icon, const QString &text, QObject *parent );
    PopupDropperAction( QSvgRenderer* renderer, const QString &elementId, const QString &text, QObject *parent );
    PopupDropperAction( QSvgRenderer* renderer, const QString &elementId, const QIcon &icon, const QString &text, QObject *parent );
    
    ~PopupDropperAction();

    QSvgRenderer* renderer() const;
    void setRenderer( QSvgRenderer *renderer );
    QString elementId() const;
    void setElementId( const QString &id );

    bool isSeparator() const;
    void setSeparator( bool separator );

    bool hasSeparatorPen() const;
    QPen separatorPen() const;
    void setSeparatorPen( const QPen &pen );
    void clearSeparatorPen();

    //quick and dirty function for getting a PopupDropperAction from a QAction.
    //no svg will be set...should only be used to transition to using PopupDropperAction
    static PopupDropperAction* from( QAction* action );

private:
    friend class PopupDropperActionPrivate;
    PopupDropperActionPrivate* const d;
};

#endif
