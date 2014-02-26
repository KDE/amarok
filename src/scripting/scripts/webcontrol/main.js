/***************************************************************************
 * copyright        : (C) 2008 Ian Monroe <ian@monroe.nu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

//Importer.load( "HttpServer.js" );


Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.network" );

QByteArray.prototype.toString = function()
{
   ts = new QTextStream( this, QIODevice.ReadOnly );
   return ts.readAll();
}

function HttpServer()
{
  QTcpServer.call( this, null );
  var portNumber = 8080;
  do {
    var connected = this.listen(new QHostAddress( QHostAddress.Any ), portNumber);
    portNumber++;
  } while( !connected && ((this.serverError() & QAbstractSocket.AddressInUseError) == 0 ) && portNumber < 9000 )
  if( !this.isListening() )
  {
    Amarok.debug( "Unable to open a port for the web server" );
    return;
  }
  Amarok.debug("Web server started at port " + this.serverPort() );
  this.newConnection.connect( this, this.newIncomingConnection );
  this.registry = new Object();
}

HttpServer.prototype = new QTcpServer();

HttpServer.prototype.newIncomingConnection = function()
{
  var socket = this.nextPendingConnection();
  var request = new QByteArray();
  var thisHttp = this;
  socket.readyRead.connect( function() {
      request.append( socket.readAll() );
      var endOfRequest =  request.indexOf("\r\n\r\n");
      if( endOfRequest > 0 )
      {
      try{
       request = thisHttp.parseHeader( request, socket, endOfRequest + 4 );
       }
       catch( error ) {
        Amarok.debug( error)
       }
      }
  });
}

HttpServer.prototype.parseHeader = function( request, socket, endOfRequest )
{
    var header = new QHttpRequestHeader( request.left( endOfRequest ).toString() );
    if( header.method() == "GET" )
    {
        this.sendResponse( socket, header.path() ); 
        socket.close();
    }
    return request.mid( endOfRequest );
}

HttpServer.prototype.sendResponse = function( socket, path )
{
   var userResponse = null;
   for( var registeredPath in this.registry )
   {
       Amarok.debug( path.indexOf( registeredPath ) + " for " + registeredPath + " in " + path );
       if( path.indexOf( registeredPath ) == 0 )
       {
           userResponse = this.registry[registeredPath]( path.substring( registeredPath.length )  );
           break;
       }
   }

   Amarok.debug( "respondTo says::" + userResponse + "::");
   if( userResponse == null )
   {
        //var response404 = new QHttpResponseHeader( 404, "Not found", 1, 1 );
//        responseHeader.setStatusLine( 404 );
        //responseHeader.setContentType( "text/html" )
        content = "HTTP/1.1 404 Not found\n"
        content += "Content-Type: text/html\n";
        content += "Server: AmarokServer\n\r\n";
        content += "<html><head><title>File not found</title></head>\n";
        content += "<h1>404 Error</h1>\n"
        content += path + " not found.\n\r\n";
        //responseHeader.setContentLength( content.length() );
        var writeMe = new QByteArray();
        //writeMe.append( response404.toString() )
        writeMe.append( content )
        socket.write( writeMe );
        Amarok.debug("response:\n" + writeMe.toString() + " sent.");
   }
   else
   {
   try{
        content = "HTTP/1.1 200\n"
        content += "Content-Type: " + userResponse.mimeType + "\n";
        content += "Server: AmarokServer\n\r\n";
        var writeMe = new QByteArray();
        writeMe.append( content );
        writeMe.append( userResponse.content );
        socket.write( writeMe );
        Amarok.debug("response:\n" + writeMe.toString() + " sent.");
    }
    catch( e )
    {
        Amarok.debug( e )
    }
   }
}

HttpServer.prototype.register = function( path, responseFn )
{
    this.registry[path] = responseFn;
    if( path.charAt(0) === "/" )
    {
        path = path.slice(1);
    }
}


function fileResponse( path )
{
    Amarok.debug( "requesting " + path );
    var prefix = "/home/ian/.kde4/share/apps/amarok/scripts/script_console2/webrok/";
    if( path === "/" || path === "" )
    {
        path = "index.js.html";
    }
    //we need to make sure it doesn't ../ out of the root
    url = new QUrl( path );
    var fi = new QFileInfo( prefix + url.path() );
    if( fi.absoluteFilePath().indexOf( prefix ) == 0 )
    {
        Amarok.debug( "sending " + fi.absoluteFilePath() );
        var file = new QFile( fi.absoluteFilePath() );
        if( file.open( QIODevice.ReadOnly ) )
        {
            var ret = new Object();
            if( fi.completeSuffix() == "xml" )
            {
                ret.mimeType = "application/xml";
            }
            else
            {
                ret.mimeType = "text/html";
            }
            ret.content = file.readAll()
            return ret;
        }
        else
        {
            Amarok.debug("file not found")
            return null;
        }
    }
    else
    {
        Amarok.debug( fi.absoluteFilePath() + " fails security check." );
        return null; //send 404 error
    }
}

function testResponse()
{
    return "<h3>test</h3>";
}

function ajaxResponse( pathStr )
{
    url = new QUrl( pathStr );
    command = url.path();
    Amarok.debug( "the command is " + command );
    switch( command ) 
    {
        case "/play":
            row = Number( url.queryItemValue( "row" ) );
            Amarok.Playlist.playByIndex( row );
            Amarok.debug("playing file");
            var ret = new Object();
            ret.mimeType = "text/plain";
            ret.content =  "Playing row " + row;
            return ret;
        break;
        case "/savePlaylist":
            Amarok.Playlist.savePlaylist("/home/ian/.kde4/share/apps/amarok/scripts/script_console2/webrok/current.xspf");
            var ret = new Object();
            ret.mimeType = "text/plain";
            ret.content =  "saving playlist"
            return ret;        
       breakZZz;
    }
    return null; //404
}

http = new HttpServer();
http.register( "/ajax", ajaxResponse );
http.register( "/test", testResponse );
http.register( "/", fileResponse );
