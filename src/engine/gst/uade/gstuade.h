// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#ifndef AMAROK_GST_GSTUADE_H
#define AMAROK_GST_GSTUADE_H

#include <gst/gst.h>

#include "uade.h"
#include "xmms-slave-msg.h"

G_BEGIN_DECLS

#define GST_TYPE_GSTUADE \
  (gst_uade_get_type())
#define GST_GSTUADE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GSTUADE,GstUade))
#define GST_GSTUADE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GSTUADE,GstUadeClass))
#define GST_IS_GSTUADE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GSTUADE))
#define GST_IS_GSTUADE_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GSTUADE))


typedef struct _GstUade GstUade;
typedef struct _GstUadeClass GstUadeClass;

struct _GstUade
{
    GstElement element;
    /* pads */
    GstPad *srcpad;

    char* streamBuf;
    int* streamBufIndex;
        
    glong blocksize; /* bytes per read */
    guint64 timeout;  /* read timeout, in nanoseconds */

    uade_msgstruct* uade_struct;
};

struct _GstUadeClass
{
    GstElementClass parent_class;

    /* signals */
    void ( *timeout ) ( GstElement *element );
};

GType gst_uade_get_type( void );
GstUade* gst_uade_new ();

G_END_DECLS


#endif /* AMAROK_GST_GSTUADE_H */



