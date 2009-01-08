/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "LayoutManager.h"

#include "Debug.h"
#include "playlist/PlaylistDefines.h"

#include <KStandardDirs>
#include <KUrl>

#include <QDomDocument>
#include <QFile>
#include <QStringList>

namespace Playlist {

LayoutManager * Playlist::LayoutManager::s_instance = 0;

LayoutManager * Playlist::LayoutManager::instance()
{
    if ( s_instance == 0 )
        s_instance = new LayoutManager();

    return s_instance;
}

LayoutManager::LayoutManager()
{
    DEBUG_BLOCK

    const KUrl url( KStandardDirs::locate( "data", "amarok/data/" ) );
    QString configFile = url.path() + "DefaultPlaylistLayouts.xml";
    loadLayouts( configFile );
    setActiveLayout( "Default" );

    debug() << "Loaded layouts: " << layouts();
    debug() << "active layout: " << m_activeLayout;
}

LayoutManager::~LayoutManager()
{
}

QStringList Playlist::LayoutManager::layouts()
{
    return m_layouts.keys();
}

void Playlist::LayoutManager::setActiveLayout( const QString & layout )
{
    m_activeLayout = layout;
}

PlaylistLayout Playlist::LayoutManager::activeLayout()
{
    return m_layouts.value( m_activeLayout );
}

void Playlist::LayoutManager::loadLayouts( const QString &fileName )
{
    QDomDocument doc( "layouts" );

    if ( !QFile::exists( fileName ) )
    {
        debug() << "file " << fileName << "does not exist";
        return;
    }

    QFile *file = new QFile( fileName );
    if( !file || !file->open( QIODevice::ReadOnly ) )
    {
        debug() << "error reading file " << fileName;
        return;
    }
    if ( !doc.setContent( file ) )
    {
        debug() << "error parsing file " << fileName;
        file->close();
        return ;
    }
    file->close();
    delete file;

    QDomElement layouts_element = doc.firstChildElement( "playlist_layouts" );
    QDomNodeList layouts = layouts_element.elementsByTagName("layout");

    int index = 0;
    while ( index < layouts.size() ) {
        QDomNode layout = layouts.item( index );
        index++;

        QString layoutName = layout.toElement().attribute( "name", "" );
        PlaylistLayout currentLayout;

        currentLayout.setHead( parseItemConfig( layout.toElement().firstChildElement( "group_head" ) ) );
        currentLayout.setBody( parseItemConfig( layout.toElement().firstChildElement( "group_body" ) ) );
        currentLayout.setSingle( parseItemConfig( layout.toElement().firstChildElement( "single_track" ) ) );

        if ( !layoutName.isEmpty() )
            m_layouts.insert( layoutName, currentLayout );
    }
}

PrettyItemConfig Playlist::LayoutManager::parseItemConfig( const QDomElement &elem )
{
    DEBUG_BLOCK
            
    bool showCover = ( elem.attribute( "show_cover", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
    int activeIndicatorRow = elem.attribute( "active_indicator_row", "0" ).toInt();

    PrettyItemConfig config;
    config.setShowCover( showCover );
    config.setActiveIndicatorRow( activeIndicatorRow );

    QDomNodeList rows = elem.elementsByTagName("row");

    int index = 0;
    while ( index < rows.size() ) {
        QDomNode rowNode = rows.item( index );
        index++;

        debug() << "row!";
        
        PrettyItemConfigRow row;

        QDomNodeList elements = rowNode.toElement().elementsByTagName("element");


        int index2 = 0;
        while ( index2 < elements.size() ) {
            QDomNode elementNode = elements.item( index2 );
            index2++;

            debug() << "element!";
            
            int value = columnNames.indexOf( elementNode.toElement().attribute( "value", "Title" ) );
            qreal size = elementNode.toElement().attribute( "size", "1.0" ).toDouble();
            bool bold = ( elementNode.toElement().attribute( "bold", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
            QString alignmentString = elementNode.toElement().attribute( "alignment", "left" );
            int alignment;
            

            if ( alignmentString.compare( "left", Qt::CaseInsensitive ) == 0 )
                alignment = Qt::AlignLeft | Qt::AlignVCenter;
            else if ( alignmentString.compare( "right", Qt::CaseInsensitive ) == 0 )
                 alignment = Qt::AlignRight| Qt::AlignVCenter;
            else
                alignment = Qt::AlignCenter| Qt::AlignVCenter;

            row.addElement( PrettyItemConfigRowElement( value, size, bold, alignment ) );
        }

        config.addRow( row );
    }

    return config;
}


}


