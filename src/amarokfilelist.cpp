/***************************************************************************
                        amarokfilelist.cpp  -  description
                           -------------------
  begin                : Nov 5 2003
  copyright            : (C) 2003 by Mark Kretschmann
  email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#include "amarokfilelist.h"

#include <kfileitem.h>
#include <kfileview.h> //compareItems

#include <qdir.h>
#include <qptrcollection.h>
#include <qstring.h> 
 

// CLASS AmarokFileList =================================================================

AmarokFileList::AmarokFileList( int i )
   : m_sortSpec( i )
{}

AmarokFileList::AmarokFileList( const KFileItemList &fil, int i )
   : KFileItemList( fil )
   , m_sortSpec( i )
{
    sort();
}

AmarokFileList::~AmarokFileList()
{}


void AmarokFileList::sort()
{
    if ( ( m_sortSpec & QDir::SortByMask ) != QDir::Unsorted )
    {
        KFileItemList::sort();
    }
}


int AmarokFileList::compareItems( QPtrCollection::Item item1, QPtrCollection::Item item2 )
{
    QString key1, key2;
    KFileItem *fileItem1 = static_cast<KFileItem*>( item1 );
    KFileItem *fileItem2 = static_cast<KFileItem*>( item2 );

    if ( ( m_sortSpec & QDir::SortByMask ) == QDir::Name )
    {
        key1 = KFileView::sortingKey( fileItem1->url().path(), fileItem1->isDir(), m_sortSpec );
        key2 = KFileView::sortingKey( fileItem2->url().path(), fileItem2->isDir(), m_sortSpec );
    }

    if ( ( m_sortSpec & QDir::SortByMask ) == QDir::Time )
    {
        key1 = KFileView::sortingKey( static_cast<KIO::filesize_t >(
                                      fileItem1->time( KIO::UDS_MODIFICATION_TIME ) ),
                                      fileItem1->isDir(), m_sortSpec );
        key2 = KFileView::sortingKey( static_cast<KIO::filesize_t >(
                                      fileItem2->time( KIO::UDS_MODIFICATION_TIME ) ),
                                      fileItem2->isDir(), m_sortSpec );
    }

    if ( ( m_sortSpec & QDir::SortByMask ) == QDir::Size )
    {
        key1 = KFileView::sortingKey( fileItem1->size(), fileItem1->isDir(), m_sortSpec );
        key2 = KFileView::sortingKey( fileItem2->size(), fileItem2->isDir(), m_sortSpec );
    }

    return key1.compare( key2 );
}
