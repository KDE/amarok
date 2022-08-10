/****************************************************************************************
 * Copyright (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "ScanningState.h"

#include <QBuffer>
#include <QDataStream>
#include <QtDebug>

#ifdef Q_CC_MSVC
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

using namespace CollectionScanner;

ScanningState::ScanningState()
        : m_sharedMemory( nullptr )
        , m_lastFilePos( 0 )
{
}

ScanningState::~ScanningState()
{
    delete m_sharedMemory;
}

void
ScanningState::setKey( const QString &key )
{
    delete m_sharedMemory;
    m_sharedMemory = new QSharedMemory( key );
    m_sharedMemory->attach();
}

bool
ScanningState::isValid() const
{
    return m_sharedMemory && m_sharedMemory->isAttached();
}

QString
ScanningState::lastDirectory() const
{
    return m_lastDirectory;
}

void
ScanningState::setLastDirectory( const QString &dir )
{
    if( dir == m_lastDirectory )
        return;

    m_lastDirectory = dir;
    writeFull();
}

QStringList
ScanningState::badFiles() const
{
    return m_badFiles;
}

void
ScanningState::setBadFiles( const QStringList &badFiles )
{
    if( badFiles == m_badFiles )
        return;

    m_badFiles = badFiles;
    writeFull();
}

QString
ScanningState::lastFile() const
{
    return m_lastFile;
}

void
ScanningState::setLastFile( const QString &file )
{
    if( file == m_lastFile )
        return;

    m_lastFile = file;

    if( !isValid() )
        return;

    QBuffer buffer;
    QDataStream out(&buffer);

    buffer.open(QBuffer::WriteOnly);

    out << m_lastFile;
    int size = buffer.size();

    if( size + m_lastFilePos < m_sharedMemory->size() )
    {
        char *to = (char*)m_sharedMemory->data();
        const char *from = buffer.data().data();
        memcpy(to + m_lastFilePos, from, size);
    }
    else
    {
        qDebug() << __PRETTY_FUNCTION__ << "QSharedMemory is too small to hold the data.";
        qDebug() << "It is of size" << m_sharedMemory->size() << "bytes but we need more than"
                 << size + m_lastFilePos << "bytes.";
    }

    m_sharedMemory->unlock();
}

void
ScanningState::readFull()
{
    if( !isValid() )
        return;

    QBuffer buffer;
    QDataStream in(&buffer);

    m_sharedMemory->lock();
    buffer.setData((char*)m_sharedMemory->constData(), m_sharedMemory->size());
    buffer.open(QBuffer::ReadOnly);

    in >> m_lastDirectory;
    in >> m_badFiles;
    m_lastFilePos = buffer.pos();
    in >> m_lastFile;

    m_sharedMemory->unlock();
}

void
ScanningState::writeFull()
{
    if( !isValid() )
        return;

    QBuffer buffer;
    QDataStream out(&buffer);
    buffer.open(QBuffer::WriteOnly);

    out << m_lastDirectory;
    out << m_badFiles;
    m_lastFilePos = buffer.pos();
    out << m_lastFile;
    int size = buffer.size();

    m_sharedMemory->lock();
    if( size < m_sharedMemory->size() )
    {
        char *to = (char*)m_sharedMemory->data();
        const char *from = buffer.data().data();
        memcpy(to, from, size);
    }
    else
    {
        qDebug() << __PRETTY_FUNCTION__ << "QSharedMemory is too small to hold the data.";
        qDebug() << "It is of size" << m_sharedMemory->size() << "bytes but we need more than"
                 << size << "bytes.";
    }

    m_sharedMemory->unlock();
}
