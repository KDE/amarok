/*************************************************
*        Debug functions                         *
*                                                *
* (c) 2008   Peter ZHOU <peterzhoulei@gmail.com> *
**************************************************/

function debug( str )
{
    var prefix;
    if (debug_prefix != "") prefix = debug_prefix;
    else prefix = "";
    var indent = " ";
    print( app_name + ":" + indent + debug_prefix + str );
}

function warning( str )
{
    var prefix;
    if (debug_prefix != "") prefix = debug_prefix;
    else prefix = "";
    var indent = " ";
    print( app_name + ":" + indent + " WARNING:" + debug_prefix + str );
}

function error( str )
{
    var prefix;
    if (debug_prefix != "") prefix = debug_prefix;
    else prefix = "";
    var indent = " ";
    print( app_name + ":" + indent + " ERROR:" + debug_prefix + str );
}

function fatal( str )
{
    var prefix;
    if (debug_prefix != "") prefix = debug_prefix;
    else prefix = "";
    var indent = " ";
    print( app_name + ":" + indent + " FATAL:" + debug_prefix + str );
}

print ( "this is debug.js" );


