// PCM time-domain equalizer
// (c) 2002 Felipe Rivera <liebremx at users sourceforge net>
// (c) 2004 Mark Kretschmann <markey@web.de>
// License: GPL V2

#ifndef AMAROK_GST_EQUALIZER_H
#define AMAROK_GST_EQUALIZER_H

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_STREAMSRC \
  (gst_equalizer_get_type())
#define GST_STREAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STREAMSRC,GstEqualizer))
#define GST_STREAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STREAMSRC,GstEqualizerClass))
#define GST_IS_STREAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STREAMSRC))
#define GST_IS_STREAMSRC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STREAMSRC))


typedef struct _GstEqualizer GstEqualizer;
typedef struct _GstEqualizerClass GstEqualizerClass;

struct _GstEqualizer
{
    GstElement element;
    /* pads */
    GstPad *srcpad;

    bool stopped;
    long curoffset;

    // Properties
    glong blocksize; /* Bytes per read */
    guint64 timeout;  /* Read timeout, in nanoseconds */

    // Pointers to member variables of GstEngine
};

struct _GstEqualizerClass
{
    GstElementClass parent_class;

    /* signals */
    void ( *timeout ) ( GstElement *element );
    void ( *kio_resume ) ( GstElement *element );
};

GType gst_equalizer_get_type( void );
GstEqualizer* gst_equalizer_new ();

G_END_DECLS


#endif /* AMAROK_GST_EQUALIZER_H */
