/*
 * Farstream Voice+Video library
 *
 *  Copyright 2011 Collabora Ltd,
 *  Copyright 2011 Nokia Corporation
 *   @author: Olivier Crete <olivier.crete@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fs-rtp-bitrate-adapter.h"
#include <gst/video/video.h>

#include <math.h>

/* This is a magical value that smarter people discovered */
#define  H264_MAX_PIXELS_PER_BIT 25

GST_DEBUG_CATEGORY_STATIC (fs_rtp_bitrate_adapter_debug);
#define GST_CAT_DEFAULT fs_rtp_bitrate_adapter_debug

static GstStaticPadTemplate fs_rtp_bitrate_adapter_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (GST_VIDEO_FORMATS_ALL)));

static GstStaticPadTemplate fs_rtp_bitrate_adapter_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (GST_VIDEO_FORMATS_ALL)));
enum
{
  PROP_0,
  PROP_BITRATE,
  PROP_INTERVAL,
};

#define PROP_INTERVAL_DEFAULT (10 * GST_SECOND)

static void fs_rtp_bitrate_adapter_finalize (GObject *object);
static void fs_rtp_bitrate_adapter_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec);


G_DEFINE_TYPE (FsRtpBitrateAdapter, fs_rtp_bitrate_adapter, GST_TYPE_ELEMENT);

static GstFlowReturn fs_rtp_bitrate_adapter_chain (GstPad *pad,
    GstObject *parent, GstBuffer *buffer);
static gboolean fs_rtp_bitrate_adapter_query (GstPad *pad, GstObject *parent,
    GstQuery *query);

static GstStateChangeReturn
fs_rtp_bitrate_adapter_change_state (GstElement *element,
    GstStateChange transition);

static void
fs_rtp_bitrate_adapter_class_init (FsRtpBitrateAdapterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = fs_rtp_bitrate_adapter_set_property;
  gobject_class->finalize = fs_rtp_bitrate_adapter_finalize;

  gstelement_class->change_state = fs_rtp_bitrate_adapter_change_state;

  GST_DEBUG_CATEGORY_INIT
      (fs_rtp_bitrate_adapter_debug, "fsrtpbitrateadapter", 0,
          "fsrtpbitrateadapter element");

  gst_element_class_set_details_simple (gstelement_class,
      "Farstream RTP Video Bitrate adater",
      "Generic",
      "Filter that can modify the resolution and framerate based"
      " on the bitrate",
      "Olivier Crete <olivier.crete@collabora.co.uk>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_bitrate_adapter_sink_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_bitrate_adapter_src_template));

  g_object_class_install_property (gobject_class,
      PROP_BITRATE,
      g_param_spec_uint ("bitrate",
          "Bitrate to adapt for",
          "The bitrate to adapt for (0 means no adaption)",
          0, G_MAXUINT, 0,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

 g_object_class_install_property (gobject_class,
      PROP_INTERVAL,
      g_param_spec_uint64 ("interval",
          "Minimum interval before adaptation",
          "The minimum interval before adapting after a change",
          0, G_MAXUINT64, PROP_INTERVAL_DEFAULT,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}

struct BitratePoint
{
  GstClockTime timestamp;
  guint bitrate;
};

static struct BitratePoint *
bitrate_point_new (GstClockTime timestamp, guint bitrate)
{
  struct BitratePoint *bp = g_slice_new (struct BitratePoint);

  bp->timestamp = timestamp;
  bp->bitrate = bitrate;

  return bp;
}

static void
bitrate_point_free (struct BitratePoint *bp)
{
  g_slice_free (struct BitratePoint, bp);
}


static void
fs_rtp_bitrate_adapter_init (FsRtpBitrateAdapter *self)
{
  self->sinkpad = gst_pad_new_from_static_template (
    &fs_rtp_bitrate_adapter_sink_template, "sink");
  gst_pad_set_chain_function (self->sinkpad, fs_rtp_bitrate_adapter_chain);
  gst_pad_set_query_function (self->sinkpad, fs_rtp_bitrate_adapter_query);
  GST_PAD_SET_PROXY_CAPS (self->sinkpad);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->srcpad = gst_pad_new_from_static_template (
    &fs_rtp_bitrate_adapter_src_template, "src");
  gst_pad_set_query_function (self->sinkpad, fs_rtp_bitrate_adapter_query);
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

  g_queue_init (&self->bitrate_history);
  self->system_clock = gst_system_clock_obtain ();
  self->interval = PROP_INTERVAL_DEFAULT;

  self->last_bitrate = G_MAXUINT;
}

static void
fs_rtp_bitrate_adapter_finalize (GObject *object)
{
  FsRtpBitrateAdapter *self = FS_RTP_BITRATE_ADAPTER (object);

  if (self->caps)
    gst_caps_unref (self->caps);

  if (self->system_clock)
    gst_object_unref (self->system_clock);

  g_queue_foreach (&self->bitrate_history, (GFunc) bitrate_point_free, NULL);
  g_queue_clear(&self->bitrate_history);

  G_OBJECT_CLASS (fs_rtp_bitrate_adapter_parent_class)->finalize (object);
}

struct Resolution {
  guint width;
  guint height;
};

static const struct Resolution one_on_one_resolutions[] =
{
  {1920, 1200},
  {1920, 1080},
  {1600, 1200},
  {1680, 1050},
  {1280, 800},
  {1280, 768},
  {1280, 720},
  {1024, 768},
  {800, 600},
  {854, 480},
  {800, 480},
  {640, 480},
  {320, 240},
  {160, 120},
  {128, 96},
  {1, 1}
};

static const struct Resolution twelve_on_eleven_resolutions[] =
{
  {1480, 1152},
  {704, 576},
  {352, 288},
  {176, 144},
  {1, 1}
};

static void
video_caps_add (GstCaps *caps, const gchar *media_type,
    guint min_framerate, guint max_framerate, guint width, guint height,
    guint par_n, guint par_d)
{
  GstStructure *s;

  s = gst_structure_new (media_type,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, par_n, par_d,
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      NULL);

  gst_structure_set (s,
      "framerate", GST_TYPE_FRACTION_RANGE, min_framerate, 1,
      max_framerate, 1, NULL);

  gst_caps_append_structure (caps, s);
}

static void
add_one_resolution_inner (GstCaps *caps, const gchar *media_type,
    guint min_framerate, guint max_framerate, guint width, guint height,
    guint par_n, guint par_d)
{
  video_caps_add (caps, media_type, min_framerate, max_framerate,
      width, height, par_n, par_d);
}

static void
add_one_resolution (const gchar *media_type, GstCaps *caps,
    GstCaps *lower_caps,
    GstCaps *extra_low_caps,
    guint max_pixels_per_second,
    guint width, guint height,
    guint par_n, guint par_d)
{
  guint pixels_per_frame = width * height;
  guint max_framerate = max_pixels_per_second / pixels_per_frame;

  /* 66 as the max framerate is a arbitrary number that I'm getting from
   * being 2/3 of 666 which is clearly evil
   */

  if (max_framerate >= 20)
  {
    add_one_resolution_inner (caps, media_type, 20, 66, width, height,
        par_n, par_d);
    add_one_resolution_inner (lower_caps, media_type, 10, 66, width, height,
        par_n, par_d);
    add_one_resolution_inner (extra_low_caps, media_type, 1, 66, width, height,
        par_n, par_d);
  }
  else if (max_framerate >= 10)
  {
    add_one_resolution_inner (lower_caps, media_type, 10, 66, width, height,
        par_n, par_d);
    add_one_resolution_inner (extra_low_caps, media_type, 1, 66, width, height,
        par_n, par_d);
  }
  else if (max_framerate > 0)
  {
    add_one_resolution_inner (extra_low_caps, media_type, 1, 66, width, height,
        par_n, par_d);
  }
}


GstCaps *
caps_from_bitrate (const gchar *media_type, guint bitrate)
{
  GstCaps *caps = gst_caps_new_empty ();
  GstCaps *lower_caps = gst_caps_new_empty ();
  GstCaps *extra_low_caps = gst_caps_new_empty ();
  GstCaps *template_caps;
  guint max_pixels_per_second = bitrate * H264_MAX_PIXELS_PER_BIT;
  gint i;

  /* At least one FPS at a very low res */
  max_pixels_per_second = MAX (max_pixels_per_second, 128 * 96);

  for (i = 0; one_on_one_resolutions[i].width > 1; i++)
    add_one_resolution (media_type, caps, lower_caps, extra_low_caps,
        max_pixels_per_second,
        one_on_one_resolutions[i].width,
        one_on_one_resolutions[i].height, 1, 1);

  for (i = 0; twelve_on_eleven_resolutions[i].width > 1; i++)
    add_one_resolution (media_type, caps, lower_caps, extra_low_caps,
        twelve_on_eleven_resolutions[i].width,
        twelve_on_eleven_resolutions[i].height,
        max_pixels_per_second, 12, 11);

  gst_caps_append (caps, lower_caps);
  if (gst_caps_is_empty (caps))
    gst_caps_append (caps, extra_low_caps);
  else
    gst_caps_unref (extra_low_caps);

  template_caps =
      gst_static_pad_template_get_caps (&fs_rtp_bitrate_adapter_sink_template);
  template_caps = gst_caps_make_writable (template_caps);
  gst_caps_append (caps, template_caps);

  return caps;
}


static GstCaps *
fs_rtp_bitrate_adapter_getcaps (FsRtpBitrateAdapter *self, GstPad *pad,
    GstCaps *filter)
{
  GstPad *otherpad;
  GstCaps *peer_caps;
  GstCaps *result;
  guint bitrate;
  guint i;

  if (pad == self->srcpad)
    otherpad = self->sinkpad;
  else
    otherpad = self->srcpad;

  peer_caps = gst_pad_peer_query_caps (otherpad, filter);

  if (gst_caps_get_size (peer_caps) == 0)
    return peer_caps;

  GST_OBJECT_LOCK (self);
  bitrate = self->last_bitrate;
  GST_OBJECT_UNLOCK (self);

  if (bitrate == G_MAXUINT)
    return peer_caps;

  result = gst_caps_new_empty ();

  for (i = 0; i < gst_caps_get_size (peer_caps); i++)
  {
    GstStructure *s = gst_caps_get_structure (peer_caps, i);

    if (g_str_has_prefix (gst_structure_get_name (s), "video/"))
    {
      GstCaps *rated_caps = caps_from_bitrate (gst_structure_get_name (s),
          bitrate);
      GstCaps *copy = gst_caps_copy_nth (peer_caps, i);

      gst_caps_set_features (rated_caps, 0,
          gst_caps_features_copy (gst_caps_get_features (peer_caps, i)));

      gst_caps_append (result, gst_caps_intersect (rated_caps, copy));
      gst_caps_unref (copy);
      gst_caps_unref (rated_caps);
    }
    else
    {
      gst_caps_append (result, gst_caps_copy_nth (peer_caps, i));
    }
  }

  return result;
}

static gboolean
fs_rtp_bitrate_adapter_query (GstPad *pad, GstObject *parent, GstQuery *query)
{
  FsRtpBitrateAdapter *self = FS_RTP_BITRATE_ADAPTER (parent);
  gboolean res;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *caps, *filter;

      gst_query_parse_caps (query, &filter);
      caps = fs_rtp_bitrate_adapter_getcaps (self, pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static GstFlowReturn
fs_rtp_bitrate_adapter_chain (GstPad *pad, GstObject *parent,
    GstBuffer *buffer)
{
  FsRtpBitrateAdapter *self = FS_RTP_BITRATE_ADAPTER (parent);
  GstFlowReturn ret;

  if (!self)
    return GST_FLOW_NOT_LINKED;

  ret = gst_pad_push (self->srcpad, buffer);

  return ret;
}


static guint
fs_rtp_bitrate_adapter_get_bitrate_locked (FsRtpBitrateAdapter *self)
{
  gdouble mean = 0;
  guint count = 0;
  gdouble S = 0;
  GList *item;
  gdouble stddev;

  for (item = self->bitrate_history.head; item ;item = item->next) {
    struct BitratePoint *bp = item->data;
    gdouble delta;

    count++;
    delta = bp->bitrate - mean;
    mean = mean + delta/count;
    S = S + delta * (bp->bitrate - mean);
  }

  if (count == 0)
    return G_MAXUINT;

  g_assert (S >= 0);
  stddev = sqrt (S/count);

  if (mean > stddev)
    return (guint) (mean - stddev);
  else
    return G_MAXUINT;
}

static void
fs_rtp_bitrate_adapter_updated_unlock (FsRtpBitrateAdapter *self)
{
  guint bitrate;
  gboolean changed = FALSE;

  bitrate = fs_rtp_bitrate_adapter_get_bitrate_locked (self);

  GST_DEBUG ("Computed average lower bitrate: %u", bitrate);
  if (bitrate == G_MAXUINT)
  {
    GST_OBJECT_UNLOCK (self);
    return;
  }
  if (bitrate > self->last_bitrate * 1.1 ||
      bitrate < self->last_bitrate * 0.9)
  {
    if (self->caps)
      gst_caps_unref (self->caps);
    self->caps = caps_from_bitrate ("video/x-raw", bitrate);
    self->last_bitrate = bitrate;
    changed = TRUE;
  }
  GST_OBJECT_UNLOCK (self);

  if (changed)
    gst_pad_push_event (self->sinkpad, gst_event_new_reconfigure ());
}

static void
fs_rtp_bitrate_adapter_cleanup_locked (FsRtpBitrateAdapter *self,
    GstClockTime now)
{
  for (;;)
  {
    struct BitratePoint *bp = g_queue_peek_head (&self->bitrate_history);

    if (bp && (bp->timestamp < now - self->interval ||
            (GST_STATE (self) != GST_STATE_PLAYING &&
                g_queue_get_length (&self->bitrate_history) > 1)))
    {
      g_queue_pop_head (&self->bitrate_history);
      bitrate_point_free (bp);
    }
    else
    {
      break;
    }
  }
}

static gboolean
clock_callback (GstClock *clock, GstClockTime now, GstClockID clockid,
    gpointer user_data)
{
  FsRtpBitrateAdapter *self = user_data;

  GST_OBJECT_LOCK (self);
  if (self->clockid == clockid)
  {
    gst_clock_id_unref (self->clockid);
  }
  else
  {
    GST_OBJECT_UNLOCK (self);
    return TRUE;
  }
  self->clockid = NULL;

  fs_rtp_bitrate_adapter_updated_unlock (self);


  return TRUE;
}

static gboolean
fs_rtp_bitrate_adapter_add_bitrate_locked (FsRtpBitrateAdapter *self,
    guint bitrate)
{
  GstClockTime now = gst_clock_get_time (self->system_clock);
  gboolean first = FALSE;

  g_queue_push_tail (&self->bitrate_history, bitrate_point_new (now, bitrate));

  first = (g_queue_get_length (&self->bitrate_history) == 1);

  fs_rtp_bitrate_adapter_cleanup_locked (self, now);

  if (!self->clockid && GST_STATE (self) == GST_STATE_PLAYING)
  {
    self->clockid = gst_clock_new_single_shot_id (self->system_clock,
        now + self->interval);
    gst_clock_id_wait_async (self->clockid,
        clock_callback, gst_object_ref (self), gst_object_unref);
  }

  return first;
}


static void
fs_rtp_bitrate_adapter_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  FsRtpBitrateAdapter *self = FS_RTP_BITRATE_ADAPTER (object);
  gboolean first = FALSE;

  GST_OBJECT_LOCK (self);
  switch (prop_id)
  {
    case PROP_BITRATE:
      first = fs_rtp_bitrate_adapter_add_bitrate_locked (self,
          g_value_get_uint (value));
      break;
    case PROP_INTERVAL:
      self->interval = g_value_get_uint64 (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  if (first)
    fs_rtp_bitrate_adapter_updated_unlock (self);
  else
    GST_OBJECT_UNLOCK (self);
}


static GstStateChangeReturn
fs_rtp_bitrate_adapter_change_state (GstElement *element,
    GstStateChange transition)
{
  FsRtpBitrateAdapter *self = FS_RTP_BITRATE_ADAPTER (element);
  GstStateChangeReturn result;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      GST_OBJECT_LOCK (self);
      if (self->clockid)
      {
        gst_clock_id_unschedule (self->clockid);
        gst_clock_id_unref (self->clockid);
      }
      self->clockid = NULL;
      GST_OBJECT_UNLOCK (self);

      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      GST_OBJECT_LOCK (self);
      if (g_queue_get_length (&self->bitrate_history))
        fs_rtp_bitrate_adapter_updated_unlock (self);
      else
        GST_OBJECT_UNLOCK (self);
      break;
    default:
      break;
  }

  if ((result =
          GST_ELEMENT_CLASS (fs_rtp_bitrate_adapter_parent_class)->change_state
          (element, transition)) == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      self->last_bitrate = G_MAXUINT;
      g_queue_foreach (&self->bitrate_history, (GFunc) bitrate_point_free,
          NULL);
      g_queue_clear(&self->bitrate_history);
      break;
    default:
      break;
  }


  return result;

 failure:
  {
    GST_ERROR_OBJECT (element, "parent failed state change");
    return result;
  }
}

GstElement *
fs_rtp_bitrate_adapter_new (void)
{
  return g_object_new (FS_TYPE_RTP_BITRATE_ADAPTER, NULL);
}
