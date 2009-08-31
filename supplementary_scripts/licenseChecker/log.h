/****************************************************************************************
 * Copyright (c) 2009 Gary Steinert <gary.steinert@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef LOG_H
#define LOG_H

#include <QList>
#include <QString>
#include <QStringList>

class LogEntry 
{

  public:
    enum Type {
      information,
      error,
      warning,
      success,
      failure
    };

    enum PrintStyle {
      plainText,
      HTML
    };

    LogEntry( QString filename, QString message, Type type );
    ~LogEntry();

    QString getFilename() { return m_filename; };
    QString getMessage() { return m_message; };
    Type getType() { return m_type; };

  private:
    QString m_filename;
    QString m_message;
    Type m_type;

};

class Log: public QList<LogEntry>
{

  public:

    class CopyHolder {
      public:
	CopyHolder( QString a, QString b )
	{
	  name = a;
	  email = b;
	};

	void addFile( QString a )
	{
	  files.append( a );
	  files.sort();
	}
	
	QString name;
	QString email;
	QStringList files;
    };

    Log() {};
    ~Log() {};

    void printFullReport( LogEntry::PrintStyle style, QString filename ) { print( style, true, true, true, true, filename ); };
    void printErrorReport( LogEntry::PrintStyle style, bool warnings, QString filename ) { print( style, true, warnings, true, false, filename ); };
    void writeShellScript( QString filename );
    void addCopyHolder( QString a, QString b, QString filename );
    void addProblemFile( QString a ) { problemFiles.append( a ); };

  private:
    void print( LogEntry::PrintStyle style, bool errors, bool warnings, bool information, bool success, QString filename );
    QList<CopyHolder> copyHolders;
    QList<QString> problemFiles;

};

#endif
