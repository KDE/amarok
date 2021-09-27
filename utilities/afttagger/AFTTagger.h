/***************************************************************************
 *   Copyright (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
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

#ifndef AFTTAGGER_H
#define AFTTAGGER_H

#include <flacfile.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <mpcfile.h>
#include <oggfile.h>
#include <tfile.h>
#include <xiphcomment.h>

#include <QCoreApplication>
#include <QStringList>
#include <QElapsedTimer>
#include <QTextStream>

/**
 * @class AFTTagger
 * @short Inserts AFT tags into directories and files
 */

class AFTTagger : public QCoreApplication
{

public:
    AFTTagger( int &argc, char **argv );

    ~AFTTagger() override {}

    void processPath( const QString &path );
    bool handleMPEG( TagLib::MPEG::File *file );
    bool handleMPC( TagLib::MPC::File *file );
    bool handleMP4( TagLib::MP4::File *file );
    bool handleOgg( TagLib::Ogg::File *file );
    bool handleFLAC( TagLib::FLAC::File *file );
    bool handleXiphComment( TagLib::Ogg::XiphComment *comment, TagLib::File *file );
    QString createCurrentUID( TagLib::File *file );
    QString createV1UID( TagLib::File *file );
    QString upgradeUID( int version, const QString &currValue );
    void readArgs();
    void displayHelp();

    bool          m_delete;
    bool          m_newid;
    bool          m_quiet;
    bool          m_recurse;
    bool          m_verbose;
    QStringList   m_fileFolderList;
    QElapsedTimer m_time;
    QTextStream   m_textStream;

};


#endif // AFTTAGGER_H

