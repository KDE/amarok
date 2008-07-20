###########################################################################
#   Amarok script for interfacing with Librivox.org.                      #
#                                                                         #
#   Copyright                                                             #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               #
#   (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                          #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         #
###########################################################################

Importer.loadQtBinding( "qt.xml" );
Importer.loadQtBinding( "qt.network" );

function onConfigure()
{
    Amarok.alert( "error", "This script does not require any configuration." );
}

function onPopulate( level, parent_id, callback, filter )
{
            filter = "_none_"

            offset = 0;
/*
            if ( args.length == 5 )
            {
                name = filter.replace( "%20", " " );
            }
            else
            {
                name = "Enter Query..."
            }
*/
            if ( level == 2 )
                print( " Populating main level..." );
                html = "The results of your query for: " + filter;
                if offset > 0
                    name = name + " ( " + offset + " - " + (offset + 100) + " )";

                Amarok.ScriptableService.insertItem( service_name, level, parent_id,  name, html, filter, "" );
                Amarok.ScriptableService.donePopulating( service_name, parent_id );

            else if ( level == 1 )
            {
                print( " Populating book level..." );
                url = "http://librivox.org/newcatalog/search_xml.php?simple=" + filter;

                #fetch results
                http = new QHttp;
                http.setHost( "librivox.org" );
                data = new QIODevice;
                http.get( url, data );
                http.close();

                #some brute force parsing....
                doc = new QDomDocument("doc");
                doc.setContent(data);
                data.close();

                elt = new QDomElement;
                elt = doc.firstChildElement( "results/book/title" );
                for ( ; !elt.isNull(); elt = nextSiblingElement( "results/book/title" ) )
                {
                    titles += ele.toText().data();
                }
                elt = doc.firstChildElement( "results/book/url" );
                for ( ; !elt.isNull(); elt = nextSiblingElement( "results/book/url" ) )
                {
                    links += ele.toText().data();
                }

                count = 0

                titles.each_with_index do |title, idx|
                    link = links[idx]

                    Amarok.ScriptableService.insertItem( service_name, level, parent_id,  title, "", link, "" );
                    count = count + 1
                end
                //tell service that all items has been added to a parent item
                Amarok.ScriptableService.donePopulating( service_name, parent_id );

            }
            else if ( level == 0 )
            {
                print( " Populating episode level..." );
                print( " url: " +  callback );

                #fetch results
                http = new QHttp;
                http.setHost( "librivox.org" );\
                data = new QIODevice;
                http.get( callback, data );
                http.close();

                doc = new QDomDocument("doc");
                doc.setContent(data);
                data.close();

                #cut result down to size a little
                startIndex = doc.index( "<ul id=\"chapters\">" )
                data = data.slice!(startIndex..data.length-1)

                #remove all <em> and </em> as they screw up simple parsing if present
                data = data.replace("<em>", "")
                data = data.replace("</em>", "")

                #get stuff we need
                data.scan(/<li>(.*?)<br\s\/>\n.*\n.*\n.*href=\"(.*?\.ogg)\">ogg\svorbis/) do |a|
                    #puts "title: " + a[0]
                    #puts "url: " + a[1]

                    title = a[0].replace( "&#8217;", "'" )
                    Amarok.ScriptableService.insertItem( service_name, level, parent_id,  title, "", "", a[1] );
                end

                Amarok.ScriptableService.donePopulating( service_name, parent_id );
            }
}

Amarok.configured.connect( Configured );

service_name = "Librivox.org";
//3 levels, query, books and episodes
levels = 3;
short_description = "Search for books from Librivox";
root_html = "Librivox service script";
Amarok.ScriptableService.initService( service_name, levels, short_description, root_html, true );

Amarok.configured.connect( onConfigure );
Amarok.ScriptableService.populate.connect( onPopulate );

//app = Qt::Application.new(ARGV)
