/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef OPMLPARSER_H
#define OPMLPARSER_H

#include "amarok_export.h"

#include <threadweaver/Job.h>

#include <QDomElement>
#include <QMap>
#include <QString>
#include <QStringList>

class AMAROK_EXPORT OpmlOutline
{
    public:
        OpmlOutline( OpmlOutline *parent = 0 );
        ~OpmlOutline();

        OpmlOutline *parent() const { return m_parent; }
        bool isRootItem() const { return m_parent == 0; }

        QMap<QString,QString> attributes() const { return m_attributes; }
        void addAttribute( const QString &key, const QString &value )
                { m_attributes.insert( key, value ); }

        QList<OpmlOutline *> children() const { return m_children; }
        void setHasChildren( bool hasChildren ) { m_hasChildren = hasChildren; }
        bool hasChildren() const { return m_hasChildren; }
        void addChild( OpmlOutline *outline ) { m_children << outline; }
        void addChildren( QList<OpmlOutline *> outlineList )
                { m_children << outlineList; }

    private:
        OpmlOutline *m_parent;
        QMap<QString,QString> m_attributes;

        bool m_hasChildren;
        QList<OpmlOutline *> m_children;
};

/**
* Parser for OPML files.
*/
class AMAROK_EXPORT OpmlParser : public ThreadWeaver::Job
{
    Q_OBJECT

public:
    static const QString OPML_MIME;
    /**
     * Constructor
     * @param fileName The file to parse 
     * @return Pointer to new object
     */
    OpmlParser( const QString &fileName );

    /**
     * The function that starts the actual work. Inherited from ThreadWeaver::Job 
     * Note the work is performed in a separate thread
     * @return Returns true on success and false on failure
     */
    void run();

    /**
     * Destructor
     * @return none
     */
    ~OpmlParser();

    /**
     * Reads, and starts parsing, file. Should not be used directly.
     * @param filename The file to read
     */
    void readConfigFile( const QString &filename );

    /**
     * Get the result of the parsing as a list of OpmlOutlines.
     * This list contains only root outlines that can be found in the <body> of the OPML.
     * The rest are children of these root items.
     */
    QList<OpmlOutline *> results() const { return m_rootOutlines; }

signals:
    /**
     * Signal emmited when parsing is complete.
     */
    void doneParsing();

    /**
     * Emitted when a new outline item is available.
     * Emitted after the attributes have been read but before any of the children is
     * available. The
     * Each child will be reported seperatly in an element.
     */
    void outlineParsed( OpmlOutline *outline );

private:
    QList<OpmlOutline *> m_rootOutlines;
    QString m_sFileName;

    void parseOpmlBody( const QDomElement &e );

    OpmlOutline *parseOutlineElement( const QDomElement &e );
};

#endif
