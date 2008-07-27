/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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

/*
    This file is meant to contain application-level exceptions, i.e. those
    that propagate outside of subsystems. For particular very specific
    exceptions, like XspfParseException for example, it makes more sense
    for them to be defined in the header of the class where they can get
    trown from. By that logic, some of the exceptions still in this file
    ought to move into the Listener subsystem.
*/

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <QString>
#include <QCoreApplication>
#include <QtGlobal>

/*************************************************************************/ /**
    Exception base class
******************************************************************************/
class LastFmException
{
public:
    //LastFmException(const char* msg) : m_what( msg ) { }
    LastFmException( const QString& msg ) : m_what( msg ) { }
    
    QString
    what() const
    { return m_what; }

    QString
    tr_what() const
    { return QCoreApplication::translate("Exception", qPrintable(m_what)); }

/*
    void
    messageBox() const
    { 
        LastMessageBox::critical( QCoreApplication::translate("Exception", "Error"),
                                  tr_what() );
    }
*/

private:
    QString m_what;
};

/*************************************************************************/ /**
    Internet connection errors
******************************************************************************/
class ConnectionException : public LastFmException
{
public:
    ConnectionException(const QString& msg) : LastFmException(msg) { }
};

/*************************************************************************/ /**
    General networking errors
******************************************************************************/
class NetworkException : public LastFmException
{
public:
    NetworkException(const QString& msg) : LastFmException(msg) { }
};

/*************************************************************************/ /**
    Used if an invalid player plugin submits tracks
******************************************************************************/
class BadClientException : public LastFmException
{
public:
    BadClientException(const QString& msg) : LastFmException(msg) { }
};

/*************************************************************************/ /**
    Used if we failed parsing a player command
******************************************************************************/
class ParseException : public LastFmException
{
public:
    ParseException(const QString& msg) : LastFmException(msg) { }
};

/*************************************************************************/ /**
    Used if we receive an invalid command from a player plugin
******************************************************************************/
class BadCommandException : public LastFmException
{
public:
    BadCommandException(const QString& msg) : LastFmException(msg) { }
};

/*************************************************************************/ /**
    General radio errors.
******************************************************************************/
class RadioException : public LastFmException
{
public:
    RadioException(const QString& msg) : LastFmException(msg) { }
};

#endif // EXCEPTIONS_H
