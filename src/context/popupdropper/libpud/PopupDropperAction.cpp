/****************************************************************************************
 * Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PopupDropperAction.h"

#include <QIcon>
#include <QPen>
#include <QString>
#include <QtDebug>
#include <QtSvg/QSvgRenderer>

class PopupDropperActionPrivate
{
public:
    PopupDropperActionPrivate()
        : renderer( 0 )
        , elementId( QString() )
        , ownRenderer( false )
        , separator( false )
        , separatorStyle( PopupDropperAction::TextSeparator )
        , hasLineSeparatorPen( false )
        , lineSeparatorPen()
    {}

    ~PopupDropperActionPrivate()
    {
        if( ownRenderer )
            delete renderer;
    }

    QSvgRenderer* renderer;
    QString elementId;
    bool ownRenderer;
    bool separator;
    PopupDropperAction::SeparatorStyle separatorStyle;
    bool hasLineSeparatorPen;
    QPen lineSeparatorPen;
};

/////////////////////////////////////////////////////////////////////////

PopupDropperAction::PopupDropperAction( QObject *parent )
    : QAction( parent )
    , d( new PopupDropperActionPrivate )
{
}

PopupDropperAction::PopupDropperAction( const QString & text, QObject * parent )
    : QAction( text, parent )
    , d( new PopupDropperActionPrivate )
{
}

PopupDropperAction::PopupDropperAction( const QIcon & icon, const QString & text, QObject * parent )
    : QAction( icon, text, parent )
    , d( new PopupDropperActionPrivate )
{
}

PopupDropperAction::PopupDropperAction( const QString &elementId, const QString &text, QObject *parent )
    : QAction( text, parent )
    , d( new PopupDropperActionPrivate )
{
    d->elementId = elementId;
}

PopupDropperAction::PopupDropperAction( const QString &elementId, const QIcon &icon, const QString &text, QObject *parent )
    : QAction( icon, text, parent )
    , d( new PopupDropperActionPrivate )
{
    d->elementId = elementId;
}

PopupDropperAction::PopupDropperAction( QSvgRenderer* renderer, const QString &text, QObject *parent )
    : QAction( text, parent )
    , d( new PopupDropperActionPrivate )
{
    d->renderer = renderer;
}

PopupDropperAction::PopupDropperAction( QSvgRenderer* renderer, const QIcon &icon, const QString &text, QObject *parent )
    : QAction( icon, text, parent )
    , d( new PopupDropperActionPrivate )
{
    d->renderer = renderer;
}

//note that the elementId cannot be used by this directly; it is only here so that you can use it as a reference
//when needing to pass it in somewhere else
PopupDropperAction::PopupDropperAction( QSvgRenderer* renderer, const QString &elementId, const QString &text, QObject *parent )
    : QAction( text, parent )
    , d( new PopupDropperActionPrivate )
{
    d->renderer = renderer;
    d->elementId = elementId;
}

//note that the elementId cannot be used by this directly; it is only here so that you can use it as a reference
//when needing to pass it in somewhere else
PopupDropperAction::PopupDropperAction( QSvgRenderer* renderer, const QString &elementId, const QIcon &icon, const QString &text, QObject *parent )
    : QAction( icon, text, parent )
    , d( new PopupDropperActionPrivate )
{
    d->renderer = renderer;
    d->elementId = elementId;
}

PopupDropperAction::~PopupDropperAction()
{
    delete d;
}

QSvgRenderer* PopupDropperAction::renderer() const
{
    return d->renderer;
}

void PopupDropperAction::setRenderer( QSvgRenderer *renderer )
{
    d->renderer = renderer;
}

void PopupDropperAction::setRenderer( const QString &file )
{
    d->renderer = new QSvgRenderer( file, this );
    d->ownRenderer = true;
}

void PopupDropperAction::setRenderer( const QByteArray &contents )
{
    d->renderer = new QSvgRenderer( contents, this );
    d->ownRenderer = true;
}

QString PopupDropperAction::elementId() const
{
    return d->elementId;
}

void PopupDropperAction::setElementId( const QString &id )
{
    if( d->renderer && !d->renderer->elementExists( id ) )
        qWarning() << "Warning: element with id " << id << " does not exist in the SVG";
    d->elementId = id;;
}

void PopupDropperAction::setText( const QString &text )
{
    QAction::setText( text );
    d->separatorStyle = PopupDropperAction::TextSeparator;
}

bool PopupDropperAction::isSeparator() const
{
    return d->separator;
}

void PopupDropperAction::setSeparator( bool separator )
{
    d->separator = separator;
    QAction::setSeparator( separator );
}

PopupDropperAction::SeparatorStyle PopupDropperAction::separatorStyle() const
{
    return d->separatorStyle;
}

void PopupDropperAction::setSeparatorStyle( PopupDropperAction::SeparatorStyle style )
{
    d->separatorStyle = style;
}

bool PopupDropperAction::hasLineSeparatorPen() const
{
    return d->hasLineSeparatorPen;
}

QPen PopupDropperAction::lineSeparatorPen() const
{
    return d->lineSeparatorPen;
}

void PopupDropperAction::setLineSeparatorPen( const QPen &pen )
{
    d->hasLineSeparatorPen = true;
    d->lineSeparatorPen = pen;
    d->separatorStyle = PopupDropperAction::LineSeparator;
}

void PopupDropperAction::clearSeparatorPen()
{
    d->hasLineSeparatorPen = false;
    d->lineSeparatorPen = QPen();
}

PopupDropperAction* PopupDropperAction::from( QAction* action )
{
    return new PopupDropperAction( QByteArray(), action->icon(), action->text(), action->parent() );
}

#include "PopupDropperAction.moc"

