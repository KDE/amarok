#! /usr/bin/env python

"""
## Usage:

help       -> scons -h
compile    -> scons
clean      -> scons -c
install    -> scons install
uninstall  -> scons -c install
configure  -> scons configure prefix=/tmp/ita debug=full

Run from a subdirectory -> scons -u
"""


## this loads the bksys modules and stuff
env = Environment( tools=['default', 'generic', 'kde3'], toolpath=['./', './bksys'])


## some settings bksys will apply to the build, eg threaded libs
env.KDEuse("environ rpath thread")


## mxcl: neater way to keep the signature-thingy-files
env.SConsignFile('bksys/signatures')


## tell scons what to build
#env.subdirs( 'src' )
env.SConscript( "src/SConscript", build_dir='build', duplicate=0 )


## The version information
env.dist('amaroK', '1.3.91')

