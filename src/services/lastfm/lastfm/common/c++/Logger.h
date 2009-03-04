 /***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>

#ifdef WIN32
    #define LOGGER_STRING std::wstring
    #define LOGGER_CHAR wchar_t
    #include <windows.h> //for CRITICAL_SECTION
#else
    #define LOGGER_STRING std::string
    #define LOGGER_CHAR char
#endif


class Logger
{
    static Logger* instance;

public:
    enum Severity
    {
        Critical = 1,
        Warning,
        Info,
        Debug
    };

    /** Sets the Logger instance to this, so only call once, and make it exist
      * as long as the application does */
    Logger( const LOGGER_CHAR* filename, Severity severity = Info );
    ~Logger();

    static Logger& the() { return *instance; }

    void log( Severity level, const std::string& message, const char* function, int line );
    void log( Severity level, const std::wstring& message, const char* function, int line );

    /** plain write + flush */
    void log( const char* message );

private:
    const Severity mLevel;
#ifdef WIN32
    CRITICAL_SECTION mMutex;
#else
    pthread_mutex_t mMutex;
#endif
    std::ofstream mFileOut;
};


/** use these to log */
#define LOG( level, msg ) { \
    std::ostringstream ss; \
    ss << msg; \
    Logger::the().log( (Logger::Severity) level, ss.str(), __FUNCTION__, __LINE__ ); }
#define LOGW( level, msg ) \
    Logger::the().log( (Logger::Severity) level, \
                       (std::wostringstream() << msg).str(), \
                       __FUNCTION__, \
                       __LINE__ );

#endif
 /***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>

#ifdef WIN32
    #define LOGGER_STRING std::wstring
    #define LOGGER_CHAR wchar_t
    #include <windows.h> //for CRITICAL_SECTION
#else
    #define LOGGER_STRING std::string
    #define LOGGER_CHAR char
#endif


class Logger
{
    static Logger* instance;

public:
    enum Severity
    {
        Critical = 1,
        Warning,
        Info,
        Debug
    };

    /** Sets the Logger instance to this, so only call once, and make it exist
      * as long as the application does */
    Logger( const LOGGER_CHAR* filename, Severity severity = Info );
    ~Logger();

    static Logger& the() { return *instance; }

    void log( Severity level, const std::string& message, const char* function, int line );
    void log( Severity level, const std::wstring& message, const char* function, int line );

    /** plain write + flush */
    void log( const char* message );

private:
    const Severity mLevel;
#ifdef WIN32
    CRITICAL_SECTION mMutex;
#else
    pthread_mutex_t mMutex;
#endif
    std::ofstream mFileOut;
};


/** use these to log */
#define LOG( level, msg ) { \
    std::ostringstream ss; \
    ss << msg; \
    Logger::the().log( (Logger::Severity) level, ss.str(), __FUNCTION__, __LINE__ ); }
#define LOGW( level, msg ) \
    Logger::the().log( (Logger::Severity) level, \
                       (std::wostringstream() << msg).str(), \
                       __FUNCTION__, \
                       __LINE__ );

#endif
