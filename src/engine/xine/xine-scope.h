/* Author: Max Howell <max.howell@methylblue.com>, (C) 2004
   Copyright: See COPYING file that comes with this distribution

   This has to be a c file or for some reason it won't link! (GCC 3.4.1)
*/

#ifndef XINESCOPE_H
#define XINESCOPE_H

/* need access to some stuff for scope time stamping */
#define METRONOM_INTERNAL

#include <sys/types.h>
#include <xine/audio_out.h>


typedef struct my_node_s MyNode;

struct my_node_s
{
    MyNode        *next;
    audio_buffer_t buf;
};


extern metronom_t *myMetronom;
extern int myChannels;
extern MyNode *myList;
extern int64_t current_vpts;

#endif
