/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "Constraint::PreventDuplicates"

#include "PreventDuplicates.h"
#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/meta/Meta.h"

#include <QSet>

#include <cmath>

Constraint*
ConstraintTypes::PreventDuplicates::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p )
        return new PreventDuplicates( xmlelem, p );
    else
        return nullptr;
}

Constraint*
ConstraintTypes::PreventDuplicates::createNew( ConstraintNode* p )
{
    if ( p )
        return new PreventDuplicates( p );
    else
        return nullptr;
}

ConstraintFactoryEntry*
ConstraintTypes::PreventDuplicates::registerMe()
{
    return new ConstraintFactoryEntry( QStringLiteral("PreventDuplicates"),
                                       i18n("Prevent Duplicates"),
                                       i18n("Prevents duplicate tracks, albums, or artists from appearing in the playlist"),
                                       &PreventDuplicates::createFromXml, &PreventDuplicates::createNew );
}

ConstraintTypes::PreventDuplicates::PreventDuplicates( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
{
    QDomAttr a;

    a = xmlelem.attributeNode( QStringLiteral("field") );
    if ( !a.isNull() ) {
        m_field = static_cast<DupeField>( a.value().toInt() );
    }
}

ConstraintTypes::PreventDuplicates::PreventDuplicates( ConstraintNode* p )
        : Constraint( p )
        , m_field( DupeTrack )
{
}

QWidget*
ConstraintTypes::PreventDuplicates::editWidget() const
{
    PreventDuplicatesEditWidget* e = new PreventDuplicatesEditWidget( m_field );
    connect( e, &PreventDuplicatesEditWidget::fieldChanged, this, &PreventDuplicates::setField );
    return e;
}

void
ConstraintTypes::PreventDuplicates::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( QStringLiteral("constraint") );
    c.setAttribute( QStringLiteral("type"), QStringLiteral("PreventDuplicates") );
    c.setAttribute( QStringLiteral("field"), QString::number( m_field ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PreventDuplicates::getName() const
{
    switch ( m_field ) {
        case DupeTrack:
            return i18n("Prevent duplicate tracks");
        case DupeArtist:
            return i18n("Prevent duplicate artists");
        case DupeAlbum:
            return i18n("Prevent duplicate albums");
    }
    return QString();
}

double
ConstraintTypes::PreventDuplicates::satisfaction( const Meta::TrackList& tl ) const
{
    int d = 0;
    QSet<Meta::TrackPtr> tracks;
    QSet<Meta::AlbumPtr> albums;
    QSet<Meta::ArtistPtr> artists;
    switch ( m_field ) {
        case DupeTrack:
            for( Meta::TrackPtr t : tl )
            {
                if ( tracks.contains(t) ) {
                    d++;
                } else {
                    tracks.insert(t);
                }
            }
            break;
        case DupeAlbum:
            for( Meta::TrackPtr t : tl )
            {
                if ( albums.contains(t->album()) ) {
                    d++;
                } else {
                    albums.insert(t->album());
                }
            }
            break;
        case DupeArtist:
            for( Meta::TrackPtr t : tl )
            {
                if ( artists.contains(t->artist()) ) {
                    d++;
                } else {
                    artists.insert(t->artist());
                }
            }
            break;
    }
            
    return exp( (double)d / -3.0 );
}

void
ConstraintTypes::PreventDuplicates::setField( const int c )
{
    m_field = static_cast<DupeField>( c );
    Q_EMIT dataChanged();
}


/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::PreventDuplicatesEditWidget::PreventDuplicatesEditWidget( const int field )
    : QWidget( nullptr )
{
    ui.setupUi( this );
    ui.comboBox_Field->setCurrentIndex( field );
}

void
ConstraintTypes::PreventDuplicatesEditWidget::on_comboBox_Field_currentIndexChanged( const int v )
{
    Q_EMIT fieldChanged( v );
    Q_EMIT updated();
}
