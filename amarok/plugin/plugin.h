//
// Author: Mark Kretschmann (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMAROK_PLUGIN_H
#define AMAROK_PLUGIN_H

/**
 * Size doesn't matter!
 */

#define AMAROK_EXPORT_PLUGIN( classname ) \
    extern "C" { \
         amaroK::Plugin* create_plugin() { return new classname; } \
    } 
    
namespace amaroK {
    
class Plugin
{
    public:
        Plugin();
        virtual ~Plugin();
};

} //namespace amaroK


#endif /* AMAROK_PLUGIN_H */


