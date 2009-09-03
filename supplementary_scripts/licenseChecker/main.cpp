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

#include "log.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QString>

const QString BLANKLINE( " *                                                                                      *" );
const QString TOPLINE( "/****************************************************************************************" );
QList<QStringList> LICENSES = QList<QStringList>() <<
    ( QStringList() <<
        " * This program is free software; you can redistribute it and/or modify it under        *" <<
        " * the terms of the GNU General Public License as published by the Free Software        *" <<
        " * Foundation; either version 2 of the License, or (at your option) any later           *" <<
        " * version.                                                                             *" <<
        " *                                                                                      *" <<
        " * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *" <<
        " * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *" <<
        " * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *" <<
        " *                                                                                      *" <<
        " * You should have received a copy of the GNU General Public License along with         *" <<
        " * this program.  If not, see <http://www.gnu.org/licenses/>.                           *"
    )
    <<
    ( QStringList() <<
        " * This program is free software; you can redistribute it and/or modify it under        *" <<
        " * the terms of the GNU Library General Public License as published by the Free         *" <<
        " * Software Foundation; either version 2.1 of the License, or (at your option) any      *" <<
        " * later version.                                                                       *" <<
        " *                                                                                      *" <<
        " * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *" <<
        " * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *" <<
        " * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *" <<
        " *                                                                                      *" <<
        " * You should have received a copy of the GNU Library General Public License along with *" <<
        " * this program.  If not, see <http://www.gnu.org/licenses/>.                           *"
    )
    <<
    ( QStringList() <<
        " * This program is free software; you can redistribute it and/or modify it under        *" <<
        " * the terms of the GNU General Public License as published by the Free Software        *" <<
        " * Foundation; either version 2 of the License, or (at your option) version 3 or        *" <<
        " * any later version accepted by the membership of KDE e.V. (or its successor approved  *" <<
        " * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *" <<
        " * version 3 of the license.                                                            *" <<
        " *                                                                                      *" <<
        " * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *" <<
        " * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *" <<
        " * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *" <<
        " *                                                                                      *" <<
        " * You should have received a copy of the GNU General Public License along with         *" <<
        " * this program.  If not, see <http://www.gnu.org/licenses/>.                           *"
    )
    <<
    ( QStringList() <<
        " * This program is free software; you can redistribute it and/or modify it under        *" <<
        " * the terms of the GNU General Public License as published by the Free Software        *" <<
        " * Foundation; either version 2 of the License, or (at your option) version 3 or any    *" <<
        " * later version publicly approved by Trolltech ASA (or its successor, if any) and the  *" <<
        " * KDE Free Qt Foundation.                                                              *" <<
        " *                                                                                      *" <<
        " * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *" <<
        " * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *" <<
        " * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *" <<
        " *                                                                                      *" <<
        " * You should have received a copy of the GNU General Public License along with         *" <<
        " * this program.  If not, see <http://www.gnu.org/licenses/>.                           *" <<
        " *                                                                                      *" <<
        " * In addition, Trolltech gives you certain additional rights as described in the       *" <<
        " * Trolltech GPL Exception version 1.2 which can be found at                            *" <<
        " * http://www.trolltech.com/products/qt/gplexception/                                   *"
    );

struct {
  QString outputFile;
  bool recursive;
  LogEntry::PrintStyle outputStyle;
  bool fullReport;
  bool help;
  QList<QString> folders;
  QString bashScriptFile;
} cliArgs;

Log log;

using namespace std;

void readFile( QString filename ) {

  bool autofixed = false;

  //Set up file variables
  QFile file( filename );
  if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    log.append( LogEntry( filename, "Could not open file", LogEntry::failure ) );
    return;
  }

  QTextStream inStream( &file );

  if( inStream.atEnd() )
  {
    log.append( LogEntry( filename, "Empty file - no license information", LogEntry::warning ) );
    return;
  }

  //Search for first comment block (search for /* at start of line)
  QString line;
  line = inStream.readLine();

  bool headerAtStart = true;
  int numLinesBeforeHeader = 0;

  while( !line.contains( "/*" ) && !inStream.atEnd() )
  {
    line = inStream.readLine();
    headerAtStart = false;
    numLinesBeforeHeader++;
  }

  int startpos = line.indexOf( "/*" );
  int endpos = line.lastIndexOf( "*/" );

  if( startpos == -1 ) //No block comments
  {
    log.append( LogEntry( filename, "No license header - could not find block comment", LogEntry::failure ) );

    log.addProblemFile( filename );

    return;
  }

  if( endpos != -1 || endpos > startpos ) //Single line comment - not license header
  {
    log.append( LogEntry( filename, "First block comment is single-line. Therefore not license header", LogEntry::failure ) );

    log.addProblemFile( filename );

    return;
  }

  //Find end of comment block
  QList<QString> header;

  header.append(line);

  while( !line.contains( "*/" ) && !inStream.atEnd() )
  {
    line = inStream.readLine();
    header.append( line );
  }

  int originalLength = header.count();


  if( !line.contains( "*/" ) )
  {
    log.append( LogEntry( filename, "No license header - could not find end of block comment", LogEntry::failure ) );

    log.addProblemFile( filename );

    return;
  }

  bool problemFile = false;

  if ( !headerAtStart )
  {
    log.append( LogEntry( filename, "License header is not at the start of the file", LogEntry::warning ) );

    problemFile = true;
  }


  //START CHECKS
  //Check first line *'s and correct length
  if( header[0] != TOPLINE )
  {
    log.append( LogEntry( filename, "First line of header incorrect", LogEntry::error ) );
    problemFile = true;
  }

  //Find first line of license
  int i = 1;

  bool  firstLineFound = false;

  while( i < header.count() )
  {
    for( int j = 0; j < LICENSES.count(); j++ )
    {
      if( header[i] == LICENSES[j][0] )
      {
        firstLineFound = true;
        break;
      }
    }

    if( firstLineFound )
      break;

    i++;
  }

  if( i == header.count() )
  {
    log.append( LogEntry( filename, "Required License wording and format not found. (Could not find match for first line)", LogEntry::failure ) );

    log.addProblemFile( filename );
    return;
  }

  bool licenseFound = false;

  //Declaring k here so that it stays around for the log entry if needed.
  int k;

  for( int j = 0; j < LICENSES.count(); j++ )
  {
    if( header.count() + i < LICENSES[j].count() )
      continue; //Too short to be header, so continue

    k = 1;
    while( k < LICENSES[j].count() && header[k+i] == LICENSES[j][k] )
      k++;

    if( k == LICENSES[j].count() )
    {
      //Check for extra lines. Means extra terms (+1 to incorporate last line)
      if( header.count() - i > LICENSES[j].count() + 1 )
        log.append( LogEntry( filename, "Extra license terms in license header.", LogEntry::information ) );

      licenseFound = true;
      break;
    }
  }

  if( !licenseFound )
  {
    log.append( LogEntry( filename, "Required license wording and format not found.", LogEntry::failure ) );
    log.addProblemFile( filename );
    return;
  }

  //Line i-1 should be a blank line
  if( header[i - 1] != BLANKLINE )
  {
    log.append( LogEntry( filename, "No blank line between copyright holders and license text. - Fixed", LogEntry::information ) );
    //Autofix
    header.insert( i, BLANKLINE );
    autofixed = true;
  }

  //Check for copyright holders (i - 1 > 1)
  if( i - 1 <= 1 )
  {
    log.append( LogEntry( filename, "No copyright holders present", LogEntry::warning ) );
    log.addProblemFile( filename );
  }

  bool overallSuccess = true;

  //For each copyright holder
  for( int j = 1; j < i - 1; j++ )
  {
    bool individualSuccess = true;

    //Check for blank line (may be at top of header for instance)
    if( header[j] == BLANKLINE )
    {
      log.append( LogEntry( filename, QString( "Blank line found in copyright holders. (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );
      header.removeAt( j );
      j--; //So that the next line isn't skipped
      autofixed = true;
      continue; //Continue because its a blank line - nothing else needs to be done with it
    }

    //Check each part of copyright line.
    //Check first 3 characters " * "
    if( header[j].mid( 0, 3 ) != " * " )
    {
      log.append( LogEntry( filename, QString( "First 3 characters of copyright line incorrect (incorrect * border) (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    //Check "Copyright"
    //From here, individualSuccess is used to skip remaining tests if one fails
    if( individualSuccess && header[j].mid( 3, 9 ) != "Copyright" )
    {
      if( header[j].mid( 3, 9 ).toLower() == "copyright" )
      {
        log.append( LogEntry( filename, QString( "Incorrect casing of \"Copyright\" (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );
        header[j] = header[j].left( 3 ) + "Copyright" + header[j].mid( 12 );
        autofixed = true;
      }
      else
      {
        log.append( LogEntry( filename, QString( "\"Copyright\" not found in proper place (Line ").append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
        individualSuccess = false;

        problemFile = true;
      }
    }

    //Check space between "Copyright" and "(c)"
    if( individualSuccess && header[j].mid( 12, 1 ) != " " )
    {
      log.append( LogEntry( filename, QString( "No space between \"Copyright\" and \"(c)\" (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    //Check "(c)"
    if( individualSuccess && header[j].mid( 13, 3 ) != "(c)" )
    {
      if( header[j].mid( 13, 3 ).toLower() == "(c)" )
      {
        log.append( LogEntry( filename, QString( "Incorrect casing of \"(c)\" (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );
        header[j] = header[j].left( 13 ) + "(c)" + header[j].mid( 16 );
        autofixed = true;
      }
      else
      {
        log.append( LogEntry( filename, QString( "\"(c)\" not found in proper place (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
        individualSuccess = false;

        problemFile = true;
      }
    }

    //Check space between "(c)" and year
    if( individualSuccess && header[j].mid( 16, 1 ) != " " )
    {
      log.append( LogEntry( filename, QString( "No space between \"(c)\" and copyright holder's name (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    //Process year
    //Check first year
    bool ok;
    int year = header[j].mid( 17, 4 ).toInt( &ok, 10 );

    if( individualSuccess && !ok )
    {
      log.append( LogEntry( filename, QString( "Year is not found in correct format. (Not a numeric value) (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    int k = 1;
    int lastyear;

    while( individualSuccess && ( header[j].mid( 17+(4*k), 1 ) == "-" || header[j].mid( 17+(4*k), 1 ) == "," ) )
    {
      lastyear = year;
      year = header[j].mid( 18+(4*k), 4 ).toInt( &ok, 10 );

      if( !ok )
      {
        log.append( LogEntry( filename, QString( "Year is not found in correct format. (Not a numeric value) (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
        individualSuccess = false;

        problemFile = true;
        break; //Leave loop so no false warning for in-order years
      }

      if( year < lastyear )
      {
        log.append( LogEntry( filename, QString( "Years are not in ascending order. (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::warning ) );
      }

      //Move on to next year
      k++;
    }

    if( individualSuccess && header[j].mid( 16+(5*k), 1 ) != " " )
    {
      log.append( LogEntry( filename, QString( "Incorrect separator character between years, or no space between year(s) and name. (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    //Find < at start of email
    int startEmail = header[j].indexOf( "<", 16+(5*k) );

    //Check for presence of name (startEmail > 18) (18 is first possible position plus a space)
    if( individualSuccess && startEmail <= 17+(5*k) )
    {
      log.append( LogEntry( filename, QString( "Copyright holder's name not present in correct place. (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    //Check for space at end of name
    if( individualSuccess && header[j].mid( startEmail - 1, 1 ) != " " )
    {
      log.append( LogEntry( filename, QString( "No space between copyright holder's name and email (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );
      //Need to check here as this could be due to not enough width for name & email
      if( header[j].at( header[j].lastIndexOf( "*" ) ) == ' ' )
      {
        header[j] = header[j].left( startEmail ) + ' ' + header[j].mid( startEmail );
        //Now remove extra space
        header[j] = header[j].left( header[j].lastIndexOf( "*" ) - 1 ) + header[j].mid( header[j].lastIndexOf( "*" ) );
        autofixed = true;
      }
    }

    //Check for spaces at start of name (shouldn't be there)
    if( individualSuccess && header[j].mid( 17+(5*k), 1 ) == " " )
    {
      log.append( LogEntry( filename, QString( "Too many spaces between copyright year and copyright holder's name (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );
      while( header[j].mid( 17+(5*k), 1 ) == " " )
      {
        header[j] = header[j].left( 17+(5*k) ) + header[j].mid( 18+(5*k) );
        header[j] = header[j].left( header[j].lastIndexOf( '*' ) ) + ' ' + header[j].mid( header[j].lastIndexOf( '*' ) );
      }
      autofixed = true;
    }

    //Check for end of email
    if( individualSuccess && header[j].lastIndexOf( ">" ) < startEmail )
    {
      log.append( LogEntry( filename, QString( "No closing brackets for email address (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::error ) );
      individualSuccess = false;

      problemFile = true;
    }

    //Check for @ followed by . in email address
    int atPos = header[j].indexOf( "@", startEmail );
    int dotPos = header[j].lastIndexOf( ".", header[j].lastIndexOf( ">" ) );

    if( individualSuccess && ( atPos == -1 || dotPos < atPos ) )
    {
      log.append( LogEntry( filename, QString( "Invalid email address (not format *@*.*) (Line " ).append( (char)(j+48) ).append( " of header)" ), LogEntry::warning ) );

    problemFile = true;
    }

    //Check for 'by So and So'
    if( individualSuccess && header[j].mid( 17+(5*k), 5 ).trimmed().toLower().startsWith( "by " ) )
    {
      log.append( LogEntry( filename, QString( "Copyright holder's name preceded by 'by'. (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );
      header[j] =
              header[j].left( header[j].toLower().indexOf( "by ", 17+(5*k) ) ) +
              header[j].mid( header[j].indexOf( "by ", 17+(5*k) ) + 3 );
      autofixed = true;
    }

    //Check length of line (stars in line at end)
    if( individualSuccess && header[j].trimmed().length() != 88 )
    {
      //Easiest way is to rewrite line
      header[j] = header[j].left( header[j].lastIndexOf( ">" ) + 1 );
      //Now add trailing spaces and *
      for ( int k = header[j].length(); k < 88; k++ )
        header[j] += ' ';

      header[j] += '*';

      log.append( LogEntry( filename, QString( "Length of copyright holder line incorrect. (Line " ).append( (char)(j+48) ).append( " of header) - Fixed" ), LogEntry::information ) );

      autofixed=true;
    }

    //Add copyright holder (only if success)
    if( individualSuccess )
    {
      log.addCopyHolder(
              header[j].mid( 17+(5*k), startEmail - (18+(5*k)) ),
              header[j].mid( startEmail + 1, header[j].lastIndexOf( ">" ) - (startEmail + 1) ),
              filename
              );
    }

    //Set overallSuccess false if this run failed
    if( !individualSuccess )
      overallSuccess = false;


  }

  if( autofixed )
  {
    inStream.seek( 0 );
    QList<QString> newFile;

    for( int i = 0; i < numLinesBeforeHeader; i++ )
    {
      newFile.append( inStream.readLine() );
    }

    newFile.append( header );

    //Skip old header
    for( int i = 0; i < originalLength; i++ )
    {
      inStream.readLine();
    }

    while( !inStream.atEnd() )
    {
      newFile.append( inStream.readLine() );
    }

    file.close();
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
    {
      log.append( LogEntry( filename, "Error opening file for writing autofixed header", LogEntry::error ) );
    }
    else
    {
      QTextStream outStream( &file );

      foreach( QString i, newFile )
        outStream << i << "\n";

      log.append( LogEntry( filename, "Some warnings autofixed", LogEntry::information ) );
    }
  }

  if( overallSuccess )
  {
    log.append( LogEntry( filename, "Success", LogEntry::success ) );

    if( problemFile )
      log.addProblemFile( filename );
  }
  else
  {
    log.append( LogEntry( filename, "Errors in copyright holders (see above errors)", LogEntry::failure ) );

    log.addProblemFile( filename );
  }

}

void processCliArgs(int argc, char** argv)
{
  cliArgs.outputFile = "";
  cliArgs.recursive = false;
  cliArgs.outputStyle = LogEntry::plainText;
  cliArgs.fullReport = true;
  cliArgs.help = false;
  cliArgs.folders = QList<QString>();
  cliArgs.bashScriptFile = "";

  for( int i = 1; i < argc; i++ ) {
    if( argv[i][0] != '-' ) //Input folder
    {
      cliArgs.folders.append( QString( argv[i] ) );
      continue;
    }

    if( argv[i][1] == 'o' ) //Output filename
    {
      if( QString( argv[i] ).length() > 2 )
      {
        cliArgs.outputFile = QString( argv[i] ).mid( 2 ).trimmed();
      }
      else
      {
        cliArgs.outputFile = QString( argv[i + 1] );
        i++;
      }
    }
    else if( argv[i][1] == 'b' ) //Output filename for bash script
    {
      if( QString( argv[i] ).length() > 2 )
      {
        cliArgs.bashScriptFile = QString( argv[i] ).mid( 2 ).trimmed();
      }
      else
      {
        cliArgs.bashScriptFile = QString( argv[i + 1] );
        i++;
      }
    }
    else if( argv[i][1] == 'r' )
      cliArgs.recursive = true;
    else if( argv[i][1] == 's' )
    {
      QString outputString;
      if( QString( argv[i] ).length() > 2 )
      {
        outputString = QString( argv[i] ).mid( 2 ).trimmed();
      }
      else
      {
        outputString = QString( argv[i + 1] );
        i++;
      }

      if( outputString.toLower() == "plaintext" )
      {
        cliArgs.outputStyle = LogEntry::plainText;
      }
      else if( outputString.toLower() == "html" )
      {
        cliArgs.outputStyle = LogEntry::HTML;
      }
      else
      {
        cliArgs.help = true;
      }
    }
    else if( argv[i][1] == 't' )
    {
      QString outputString;
      if( QString( argv[i] ).length() > 2 )
      {
        outputString = QString( argv[i] ).mid( 2 ).trimmed();
      }
      else
      {
        outputString = QString( argv[i + 1] );
        i++;
      }

      if( outputString.toLower() == "full" )
      {
        cliArgs.fullReport = true;
      }
      else if( outputString.toLower() == "errors" )
      {
        cliArgs.fullReport = false;
      }
      else
      {
        cliArgs.help = true;
      }
    }
    else if( argv[i][1] == 'h' )
      cliArgs.help = true;
    else
      cliArgs.help = true;
  }
}

QString appendTrailingForwardSlash( const QString a )
{
    if( !a.endsWith( '/' ) )
        return a + '/';
    else
        return a;
}

void iterateFolder( QString folder )
{
  QDir dir( folder );
  QStringList fileFilter;
  fileFilter << "*.h" << "*.cpp";
  QStringList files = dir.entryList( fileFilter, QDir::Files );

  if( cliArgs.recursive )
  {
    QStringList folders = dir.entryList( QStringList(), QDir::Dirs | QDir::NoDotAndDotDot );
    foreach( QString i, folders )
    {
      iterateFolder( appendTrailingForwardSlash( folder ) + i );
    }

  }

  foreach( QString i, files )
  {
    readFile( appendTrailingForwardSlash( folder ) + i );
  }
}

int main( int argc, char** argv )
{
  processCliArgs(argc, argv);

  if( cliArgs.help )
  {
    QTextStream output( stdout );
    output << "Amarok License Header Checker" << "\n";
    output << "Usage: " << argv[0] << " -hr -o <filename> -s <style> -t <type> file1, file2..." << "\n\n";
    output << "Command Line Options:" << "\n";
    output << "  -h             Print this help message and exit" << "\n";
    output << "  -r             Recursive processing of directories" << "\n";
    output << "  -o <filename>  Output written to <filename>" << "\n";
    output << "  -s <style>     Output format. One of plaintext, HTML. Plaintext is default" << "\n";
    output << "  -t <type>      Output type. One of full, errors. Full is default" << "\n";
    output << "  -b <filename>  Write bash script for editing erroneous files to <filename>" << "\n";

    return 0;
  }

  if( cliArgs.folders.count() == 0 )
    cliArgs.folders.append( QString() );

  foreach( QString i, cliArgs.folders )
  {
    iterateFolder( i );
  }

  if( cliArgs.fullReport )
    log.printFullReport( cliArgs.outputStyle, cliArgs.outputFile );
  else
    log.printErrorReport( cliArgs.outputStyle, true, cliArgs.outputFile );

  if( !cliArgs.bashScriptFile.isEmpty() )
    log.writeShellScript( cliArgs.bashScriptFile );

  return 0;
}

