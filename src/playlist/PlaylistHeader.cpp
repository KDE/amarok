/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"

#include "PlaylistHeader.h"

#include <KLocale>

#include <QDrag>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QStringList>
#include <QVBoxLayout>

using namespace PlaylistNS;

const QString HeaderWidget::HeaderMimeType = "application/x-amarok-playlist-header";

HeaderWidget::HeaderWidget( QWidget* parent )
    : QWidget( parent )
{
    m_test << i18n("Artist") << i18n("Track Number - Title") << i18n("Album") << i18n("Length");
    QHBoxLayout* topLayout = new QHBoxLayout( this );
    for( int i = 0; i < 2; i++ )
    {
        m_verticalLayouts.push_back( new QVBoxLayout( this ) );
        topLayout->addLayout( m_verticalLayouts.at(i) );
    }
    for( int i = 0; i < 4; i++ )
    {
        m_labels.push_back( new QLabel( this ) );
        m_labelToIndex[ m_labels[i] ] = i;
        int column = (i < 2) ? 0 : 1;
        m_verticalLayouts[column]->addWidget( m_labels.at(i) );
        m_labels.at(i)->setText( m_test.at( i ) );
    }
    setAcceptDrops( true );
}

void
HeaderWidget::mousePressEvent(QMouseEvent *event)
{
    QLabel *child = dynamic_cast<QLabel*>(childAt(event->pos()));
    if (!child)
        return;
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << child->text();
    QMimeData* mimeData = new QMimeData;
    mimeData->setData( HeaderMimeType, itemData );
    QDrag* drag = new QDrag( this );
    QPixmap labelPixmap;
    labelPixmap.grabWidget( child );
    drag->setPixmap( labelPixmap );
    drag->setMimeData( mimeData );
    drag->exec();
}

#include "PlaylistHeader.moc"
