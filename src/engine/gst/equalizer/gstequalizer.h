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

    int band_count;

    // Properties
    glong blocksize; /* Bytes per read */
    guint64 timeout;  /* Read timeout, in nanoseconds */

    float gain[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
    float preamp[EQ_CHANNELS] __attribute__((aligned));

    /* Floating point */
    typedef struct
    {
        float beta;
        float alpha;
        float gamma;
    } sIIRCoefficients;

    /* Coefficient history for the IIR filter */
    typedef struct
    {
        float x[3]; /* x[n], x[n-1], x[n-2] */
        float y[3]; /* y[n], y[n-1], y[n-2] */
    } sXYData;
};

struct _GstEqualizerClass
{
    GstElementClass parent_class;

    /* signals */
};


static void gst_equalizer_set_property ( GObject * object, guint prop_id,
                                         const GValue * value, GParamSpec * pspec );

static void gst_equalizer_get_property ( GObject * object, guint prop_id,
                                         GValue * value, GParamSpec * pspec );

static GstElementStateReturn gst_equalizer_change_state (GstElement* element);

static void clean_history();
static void set_filters( GstEqualizer* obj, gint bands, gint sfreq );
static GstData *gst_equalizer_chain ( GstPad* pad, GstData* data );

GType gst_equalizer_get_type( void );
GstEqualizer* gst_equalizer_new ();

G_END_DECLS


#endif /* AMAROK_GST_EQUALIZER_H */
