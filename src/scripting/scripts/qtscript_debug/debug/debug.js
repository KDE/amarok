/*************************************************
*        Debug functions                         *
*                                                *
* (c) 2008   Peter ZHOU <peterzhoulei@gmail.com> *
**************************************************/

Debug.initialize = function ( key )
{
    this.debug_prefix = "";
    this.app_name = "";
}

Debug.app_name = function ( str )
{
    this.app_name = str;
}

Debug.debug = function ( str )
{
    if ( this.debug_prefix != "" ) this.prefix = this.debug_prefix;
    else this.prefix = "";
    this.indent = " ";
    Amarok.debug( this.app_name + ":" + this.indent + this.prefix + str );
}

Debug.warning = function ( str )
{
    if ( this.debug_prefix != "" ) this.prefix = this.debug_prefix;
    else this.prefix = "";
    this.indent = " ";
    Amarok.debug( this.app_name + ":" + this.indent + " WARNING:" + this.prefix + str );
}

Debug.error = function ( str )
{
    if ( this.debug_prefix != "" ) this.prefix = this.debug_prefix;
    else this.prefix = "";
    this.indent = " ";
    Amarok.debug( this.app_name + ":" + this.indent + " ERROR:" + this.prefix + str );
}

Debug.fatal = function ( str )
{
    if ( this.debug_prefix != "" ) this.prefix = this.debug_prefix;
    else this.prefix = "";
    this.indent = " ";
    Amarok.debug( this.app_name + ":" + this.indent + " FATAL:" + this.prefix + str );
}


