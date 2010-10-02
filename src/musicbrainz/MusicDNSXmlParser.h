/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICDNSXMLPARSER_H
#define MUSICDNSXMLPARSER_H

#include <QDomDocument>
#include <QStringList>

#include <threadweaver/Job.h>

class MusicDNSXmlParser : public ThreadWeaver::Job
{
    public:
        MusicDNSXmlParser(QString &doc );
        void run();
        QStringList puid();

    private:
        void parseElement( const QDomElement &e );
        void parseChildren( const QDomElement &e );

        void parseTrack( const QDomElement &e );

        void parsePUIDList( const QDomElement &e );
        void parsePUID( const QDomElement &e );

        QDomDocument m_doc;
        QStringList m_puid;
};

#endif // MUSICDNSXMLPARSER_H
