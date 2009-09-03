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
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#define INDENT "    "

#include "log.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

LogEntry::LogEntry( QString filename, QString message, LogEntry::Type type = LogEntry::information )
{
  m_filename = filename;
  m_message = message;
  m_type = type;
}

LogEntry::~LogEntry()
{
}

void Log::print(LogEntry::PrintStyle style, bool errors, bool warnings, bool information, bool success, QString outputFile)
{
  //Get number of failed files, warnings and autofixes
  int failedCount = 0;
  int warningCount = 0;
  int autoFixCount = 0;
  
  foreach( LogEntry i, *this )
  {
    if( i.getType() == LogEntry::failure )
      failedCount++;
    if( i.getType() == LogEntry::warning )
      warningCount++;
    if( i.getType() == LogEntry::information && i.getMessage().endsWith( " - Fixed" ) )
      autoFixCount++;
  }
    
  
  
  QFile filePtr( outputFile );
  bool opened;

  if( outputFile.isEmpty() )
    opened = filePtr.open( stdout, QFile::WriteOnly );
  else
    opened = filePtr.open( QFile::WriteOnly | QFile::Truncate );
  
  if( !opened )
  {
    //Cant print an error message as its possible it was stdout that failed
    return;
  }

  QTextStream output( &filePtr );
  
  if( style == LogEntry::plainText )
  {
    //Print header
    output << "AMAROK LICENSE HEADER CHECKER REPORT\n";
    output << "Date: " << QDateTime::currentDateTime().toString() << "\n";
    output << "------------------------------------\n\n";

    //Print contents
    QString lastFile = "";

    foreach( LogEntry i, *this )
    {
      //Skip if not to be included
      if( ( i.getType() == LogEntry::error && !errors ) ||
	  ( i.getType() == LogEntry::failure && !errors ) ||
	  ( i.getType() == LogEntry::warning && !warnings ) ||
	  ( i.getType() == LogEntry::success && !success ) ||
	  ( i.getType() == LogEntry::information && !information ) )
      {
	continue;
      }

      //Print filename if first entry for file
      if( i.getFilename() != lastFile )
      {
	output << i.getFilename() << "\n";
      }

      //Print type & message
      QString typeSymbol;

      if( i.getType() == LogEntry::information )  typeSymbol = "[I]";
      else if( i.getType() == LogEntry::error )   typeSymbol = "[E]";
      else if( i.getType() == LogEntry::warning ) typeSymbol = "[W]";
      else if( i.getType() == LogEntry::success ) typeSymbol = "[S]";
      else if( i.getType() == LogEntry::failure ) typeSymbol = "[F]";
      else                                        typeSymbol = "[?]";

      output << INDENT << typeSymbol << i.getMessage() << "\n";

      lastFile = i.getFilename();

    }

    //Print footer
    output << "-----------------------------\n";
    output << "END OF REPORT\n";
  }

  if( style == LogEntry::HTML )
  {
    //Print header
    output << "<html>" << "\n";
    output << "<head>" << "\n";
    output << "<style>" << "\n";
    output << "table {" << "\n";
    output << "  color: #7F7F7F;" << "\n";
    output << "  font: 0.8em/1.6em \"Trebuchet MS\",Verdana,sans-serif;" << "\n";
    output << "  border-collapse: collapse" << "\n";
    output << "}" << "\n\n";
    
    output << "table,caption {" << "\n";
    output << "  margin: 0 auto;" << "\n";
    output << "border-right: 1px solid #CCC;" << "\n";
    output << "  border-left: 1px solid #CCC" << "\n";
    output << "}" << "\n\n";
    
    output << "caption,th,td {" << "\n";
    output << "  border-left: 0;" << "\n";
    output << "  padding: 10px" << "\n";
    output << "}" << "\n\n";

    output << "caption,thead th,tfoot th,tfoot td {" << "\n";
    output << "  background-color: #E63C1E;" << "\n";
    output << "  color: #FFF;" << "\n";
    output << "  font-weight: bold;" << "\n";
    output << "  text-transform: uppercase" << "\n";
    output << "}" << "\n\n";

    output << "thead th {" << "\n";
    output << "  background-color: #C30;" << "\n";
    output << "  color: #FFB3A6;" << "\n";
    output << "  text-align: center" << "\n";
    output << "}" << "\n\n";

    output << "tbody th {" << "\n";
    output << "  padding: 20px 10px" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr.green {" << "\n";
    output << "  background-color: #D0FFD0;" << "\n";
    output << "  color: #666" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr.yellow {" << "\n";
    output << "  background-color: #FFFFD0;" << "\n";
    output << "  color: #666" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr.red {" << "\n";
    output << "  background-color: #FFD0D0;" << "\n";
    output << "  color: #666" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr.blue {" << "\n";
    output << "  background-color: #D0D0FF;" << "\n";
    output << "  color: #666" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr.header {" << "\n";
    output << "  background-color: #D0D0D0;" << "\n";
    output << "  color: #666" << "\n";
    output << "  font-weight: bold" << "\n";
    output << "}" << "\n\n";

    output << "tbody a {" << "\n";
    output << "  padding: 1px 2px;" << "\n";
    output << "  color: #333;" << "\n";
    output << "  text-decoration: none;" << "\n";
    output << "  border-bottom: 1px dotted #E63C1E" << "\n";
    output << "}" << "\n\n";

    output << "tbody a:active,tbody a:hover,tbody a:focus,tbody a:visited {" << "\n";
    output << "  color: #666" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr:hover {" << "\n";
    output << "  background-color: #EEE;" << "\n";
    output << "  color: #333" << "\n";
    output << "}" << "\n\n";

    output << "tbody tr:hover a {" << "\n";
    output << "  background-color: #FFF" << "\n";
    output << "}" << "\n\n";

    output << "tbody td+td+td+td a {" << "\n";
    output << "  color: #C30;" << "\n";
    output << "  font-weight: bold;" << "\n";
    output << "  border-bottom: 0" << "\n";
    output << "}" << "\n\n";

    output << "tbody td+td+td+td a:active,tbody td+td+td+td a:hover,tbody td+td+td+td a:focus,tbody td+td+td+td a:visited {" << "\n";
    output << "  color: #E63C1E" << "\n";
    output << "}" << "\n";

    output << "</style>" << "\n";
    output << "<script type=\"text/javascript\">" << "\n";
    output << "function showFiles(id, count) {" << "\n";
    output << "  if(document.getElementById(id + \".0\").style.display == 'table-row')" << "\n";
    output << "    newDisplay = 'none';" << "\n";
    output << "  else" << "\n";
    output << "    newDisplay = 'table-row';" << "\n";
    output << "  for (i=0; i<count; i++) {" << "\n";
    output << "    document.getElementById(id + \".\" + i).style.display = newDisplay;" << "\n";
    output << "  }" << "\n";
    output << "}" << "\n";
    output << "</script>" << "\n";
    output << "<title>Amarok License Checker Report - " << QDateTime::currentDateTime().toString() << "</title>" << "\n";
    output << "</head>" << "\n";
    output << "<body>" << "\n";
    output << "<h1>Amarok License Checker Report</h1>" << "\n";
    output << "<h3>Report Date: " << QDateTime::currentDateTime().toString() << "</h3>" << "\n";
    output << "<table border=\"1\">" << "\n";
    output << "<tr class=\"header\"><td colspan=\"2\">General Information:</td></tr>" << "\n";
    output << "<tr><td>Number of failures:</td><td>" << failedCount << "</td></tr>" << "\n";
    output << "<tr><td>Number of warnings:</td><td>" << warningCount << "</td></tr>" << "\n";
    output << "<tr><td>Number of autofixes:</td><td>" << autoFixCount << "</td></tr>" << "\n";
    output << "</table>" << "\n";
    output << "<p></p>" << "\n";
    output << "<table border=\"1\">" << "\n";
    
    //Print contents
    QString lastFile = "";

    foreach( LogEntry i, *this )
    {
      //Skip if not to be included
      if( ( i.getType() == LogEntry::error && !errors ) ||
	  ( i.getType() == LogEntry::failure && !errors ) ||
	  ( i.getType() == LogEntry::warning && !warnings ) ||
	  ( i.getType() == LogEntry::success && !success ) ||
	  ( i.getType() == LogEntry::information && !information ) )
      {
	continue;
      }

      //Print filename if first entry for file
      if( i.getFilename() != lastFile )
      {
	output << "<tr>" << "\n";
	output << "<td colspan=\"2\">" << i.getFilename() << "</td>" << "\n";
	output << "</tr>" << "\n";
      }

      //Print type & message
      QString typeSymbol;

      if( i.getType() == LogEntry::information )  typeSymbol = "[I]";
      else if( i.getType() == LogEntry::error )   typeSymbol = "[E]";
      else if( i.getType() == LogEntry::warning ) typeSymbol = "[W]";
      else if( i.getType() == LogEntry::success ) typeSymbol = "[S]";
      else if( i.getType() == LogEntry::failure ) typeSymbol = "[F]";
      else                                        typeSymbol = "[?]";

      QString rowClass;

      if( i.getType() == LogEntry::error || i.getType() == LogEntry::failure )
	rowClass = "red";
      else if( i.getType() == LogEntry::warning )
	rowClass = "yellow";
      else if( i.getType() == LogEntry::success )
	rowClass = "green";
      else if( i.getType() == LogEntry::information )
	rowClass = "blue";

      output << "<tr class=\"" << rowClass << "\">" << "\n";
      output << "<td style=\"width:10px\">" << typeSymbol << "</td>" << "\n";
      output << "<td>" << i.getMessage() << "</td>" << "\n";
      output << "</tr>" << "\n";

      lastFile = i.getFilename();

    }

    //Print files table footer and copyHolders table header
    output << "</table>" << "\n";
    output << "<p></p>" << "\n";
    output << "<table border=\"1\">" << "\n";
    output << "<tr class=\"header\"><td colspan=\"2\" align=\"center\">Copyright Holders" << "\n";
    if( !information )
      output << "<br>(Duplicate names shown only)" << "\n";
    output << "<br>Click on row to view files</td></tr>" << "\n";
    output << "<tr class=\"header\"><td>Name</td><td>Email</td></tr>" << "\n";
    
    for( int i = 0; i < copyHolders.count(); i++ )
    {
      //Beginning and end require special cases
      //Disaply all if information==true
      if( information ||
	  ( i == 0 && copyHolders[i + 1].name == copyHolders[i].name ) ||
	  ( i == copyHolders.count() - 1 && copyHolders[i - 1].name == copyHolders[i].name ) ||
	  ( ( i > 0 && i < copyHolders.count() - 1 ) && ( copyHolders[i + 1].name == copyHolders[i].name || copyHolders[i - 1].name == copyHolders[i].name ) ) )
      {
	output << "<tr onClick=\"showFiles('filesRow" << i << "'," << copyHolders[i].files.count() << ")\">" << "\n";
	output << "<td>" << copyHolders[i].name << "</td>" << "\n";
	output << "<td>" << copyHolders[i].email << "</td>" << "\n";
	output << "</tr>" << "\n";
	
	for( int j = 0; j < copyHolders[i].files.count(); j++ )
	{
	  output << "<tr class=\"yellow\" id=\"filesRow" << i << "." << j << "\" style=\"display:none\">" << "\n";
	  output << "<td colspan=2>" << copyHolders[i].files[j] << "</td>" << "\n";
	  output << "</tr>" << "\n";
	}
      }
	
    }

    //Print footer
    output << "</table>" << "\n";
    output << "</body>" << "\n";
    output << "</html>" << "\n";
  }
}

void Log::addCopyHolder( QString a, QString b, QString filename )
{
  //If empty list no search required
  if( copyHolders.isEmpty() )
  {
    copyHolders.append( CopyHolder( a , b ) );
    return;
  }
  
  //Search for same name
  //Using a binary search to check for existence then an incremental search
  //to find insertion point as I have had trouble implementing it off the
  //binary search results
  int start, end, mid;
  start = 0;
  end = copyHolders.count() - 1;
  mid = 0;
  int insert = -1;
  
  //This is a change to a normal binary search
  //Because i want to insert when things are NOT found, i need to run an
  //extra iteration. start<=end is kept in place for end < start eventualities
  //and as start==end situations either turn into start>end situations or result
  //in a found entry (and therefore either a return or break) no checks for start==end
  //are required
  while( start <= end )
  {
    mid = (int)( ( start + end ) / 2 );
    
    if( copyHolders[mid].name.toLower() == a.toLower() )
    {
      //Might not have found the first entry for the name, so cycle back
      while( mid >= 0 && copyHolders[mid].name.toLower() == a.toLower() )
	mid--;
      
      //Now mid++ to select the first entry for name
	mid++;
      
      while( mid < copyHolders.count() && copyHolders[mid].name.toLower() == a.toLower() && copyHolders[mid].email.toLower() != b.toLower() )
      {
	mid++;
      }

      //Checking name here because if email happens to be the same for the next entry
      //(different name) then entry will not be added
      if( mid < copyHolders.count() && copyHolders[mid].name.toLower() == a.toLower() )
      {
	//If found, no need to add so add file name and exit function
	copyHolders[mid].addFile( filename );
	return;
      }
      else
      {
	//Found the insertion point
	insert = mid;
	break;
      }
    }
    else
    {
      if( copyHolders[mid].name.toLower() < a.toLower() )
	start = mid + 1;
      else
	end = mid - 1;
    }
    
  }
  
  mid = (int)( ( start + end ) / 2 );
  
  if( insert > -1 )
  {
    copyHolders.insert( insert, CopyHolder( a, b ) );
    copyHolders[insert].addFile( filename );
  }
  else if( copyHolders[mid].name < a )
  {
    copyHolders.insert( mid + 1, CopyHolder( a, b ) );
    copyHolders[mid + 1].addFile( filename );
  }
  else
  {
    copyHolders.insert( mid, CopyHolder( a, b ) );
    copyHolders[mid].addFile( filename );
  }
}

void Log::writeShellScript( QString filename )
{
  QFile filePtr( filename );
  
  if( !filePtr.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
  {
    return;
  }
  
  QTextStream output( &filePtr );
  
  output << "#!/bin/bash" << "\n";
  
  int count = 1; 

  foreach( QString i, problemFiles )
  {
    output << "echo \"Next file: " << i << "\"" << "\n";
    output << "echo \"Press enter to edit, type 'exit' to end script\"" << "\n";
    output << "read EXIT" << "\n";
    output << "if [ \"$EXIT\" = \"exit\" ]; then" << "\n";
    output << "  exit 0;" << "\n";
    output << "fi" << "\n";
    output << " echo \"Files processed so far: " << count << "\"" << "\n";
    count++;
    output << "vim " << i << "\n";
  }
}
  
