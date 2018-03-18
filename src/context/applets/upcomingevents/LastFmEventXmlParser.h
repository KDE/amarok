/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_LASTFMEVENTXMLPARSER_H
#define AMAROK_LASTFMEVENTXMLPARSER_H

#include "LastFmEvent.h"

#include <QString>
#include <QXmlStreamReader>

class LastFmEventXmlParser
{
public:
    explicit LastFmEventXmlParser( QXmlStreamReader &reader );
    ~LastFmEventXmlParser();

    bool read();
    LastFmEvent::List events() const { return m_events; }
    LastFmEvent::List events() { return m_events; }

private:
    QHash<QString, QString> readEventArtists();
    QStringList readEventTags();

    QXmlStreamReader &m_xml;
    LastFmEvent::List m_events;
    Q_DISABLE_COPY( LastFmEventXmlParser )
};

class LastFmVenueXmlParser
{
public:
    explicit LastFmVenueXmlParser( QXmlStreamReader &reader );
    ~LastFmVenueXmlParser();

    bool read();
    LastFmVenuePtr venue() const { return m_venue; }
    LastFmVenuePtr venue() { return m_venue; }

private:
    LastFmVenuePtr m_venue;
    QXmlStreamReader &m_xml;
    Q_DISABLE_COPY( LastFmVenueXmlParser )
};

class LastFmLocationXmlParser
{
public:
    explicit LastFmLocationXmlParser( QXmlStreamReader &reader );
    ~LastFmLocationXmlParser();

    bool read();

    LastFmLocationPtr location() const { return m_location; }
    LastFmLocationPtr location() { return m_location; }

private:
    void readGeoPoint();

    LastFmLocationPtr m_location;
    QXmlStreamReader &m_xml;
    Q_DISABLE_COPY( LastFmLocationXmlParser )
};

#endif /* AMAROK_LASTFMEVENTXMLPARSER_H */
