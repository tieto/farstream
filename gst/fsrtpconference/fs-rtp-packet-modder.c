/*
 * Farstream - Farstream RTP Packet modder
 *
 * Copyright 2010 Collabora Ltd.
 *  @author: Olivier Crete <olivier.crete@collabora.co.uk>
 * Copyright 2010 Nokia Corp.
 *
 * fs-rtp-packet-modder.c - Filter to modify RTP packets
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
#  include "config.h"
#endif

#include "fs-rtp-packet-modder.h"

GST_DEBUG_CATEGORY_STATIC (fs_rtp_packet_modder_debug);
#define GST_CAT_DEFAULT fs_rtp_packet_modder_debug

static GstStaticPadTemplate fs_rtp_packet_modder_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ("application/x-rtp"));

static GstStaticPadTemplate fs_rtp_packet_modder_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ("application/x-rtp"));

G_DEFINE_TYPE (FsRtpPacketModder, fs_rtp_packet_modder, GST_TYPE_ELEMENT);

static GstFlowReturn fs_rtp_packet_modder_chain (GstPad *pad,
    GstObject *parent, GstBuffer *buffer);
static GstCaps *fs_rtp_packet_modder_getcaps (FsRtpPacketModder *self,
    GstPad *pad, GstCaps *filter);
static gboolean fs_rtp_packet_modder_sink_event (GstPad *pad,
    GstObject *parent,
    GstEvent *event);
static gboolean fs_rtp_packet_modder_query (GstPad *pad,
    GstObject *parent,
    GstQuery *query);
static GstStateChangeReturn fs_rtp_packet_modder_change_state (
  GstElement *element, GstStateChange transition);


static void
fs_rtp_packet_modder_class_init (FsRtpPacketModderClass *klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT
      (fs_rtp_packet_modder_debug, "fsrtppacketmodder", 0,
          "fsrtppacketmodder element");

  gst_element_class_set_details_simple (gstelement_class,
      "Farstream RTP Packet modder",
      "Generic",
      "Filter that can modify RTP packets",
      "Olivier Crete <olivier.crete@collabora.co.uk>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_packet_modder_sink_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_packet_modder_src_template));

  gstelement_class->change_state = fs_rtp_packet_modder_change_state;
}

static void
fs_rtp_packet_modder_init (FsRtpPacketModder *self)
{
  gst_segment_init (&self->segment, GST_FORMAT_TIME);

  self->sinkpad = gst_pad_new_from_static_template (
    &fs_rtp_packet_modder_sink_template, "sink");
  gst_pad_set_chain_function (self->sinkpad, fs_rtp_packet_modder_chain);
  gst_pad_set_query_function (self->sinkpad, fs_rtp_packet_modder_query);
  gst_pad_set_event_function (self->sinkpad, fs_rtp_packet_modder_sink_event);
  GST_PAD_SET_PROXY_CAPS (self->sinkpad);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->srcpad = gst_pad_new_from_static_template (
    &fs_rtp_packet_modder_src_template, "src");
  gst_pad_set_query_function (self->srcpad, fs_rtp_packet_modder_query);
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);
}

FsRtpPacketModder *
fs_rtp_packet_modder_new (FsRtpPacketModderFunc modder_func,
    FsRtpPacketModderSyncTimeFunc sync_func,
    gpointer user_data)
{
  FsRtpPacketModder *self;

  g_return_val_if_fail (modder_func != NULL, NULL);
  g_return_val_if_fail (sync_func != NULL, NULL);

  self = g_object_new (FS_TYPE_RTP_PACKET_MODDER, NULL);

  self->modder_func = modder_func;
  self->sync_func = sync_func;
  self->user_data = user_data;

  return self;
}

static void
fs_rtp_packet_modder_sync_to_clock (FsRtpPacketModder *self,
  GstClockTime buffer_ts)
{
  GstClockTime running_time;
  GstClockTime sync_time;
  GstClockID id;
  GstClock *clock;
  GstClockReturn clockret;

  GST_OBJECT_LOCK (self);
  running_time = gst_segment_to_running_time (&self->segment, GST_FORMAT_TIME,
     buffer_ts);

  do {
    sync_time = running_time + GST_ELEMENT_CAST (self)->base_time +
        self->peer_latency;

    clock = GST_ELEMENT_CLOCK (self);
    if (!clock) {
      GST_OBJECT_UNLOCK (self);
      /* let's just push if there is no clock */
      GST_LOG_OBJECT (self, "No clock, push right away");
      return;
    }

    GST_LOG_OBJECT (self, "sync to running timestamp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (running_time));

    id = self->clock_id = gst_clock_new_single_shot_id (clock, sync_time);
    self->unscheduled = FALSE;
    GST_OBJECT_UNLOCK (self);

    clockret = gst_clock_id_wait (id, NULL);

    GST_OBJECT_LOCK (self);
    gst_clock_id_unref (id);
    self->clock_id = NULL;

  } while  (clockret == GST_CLOCK_UNSCHEDULED && !self->unscheduled);
  GST_OBJECT_UNLOCK (self);
}

static GstFlowReturn
fs_rtp_packet_modder_chain (GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
  FsRtpPacketModder *self = FS_RTP_PACKET_MODDER (parent);
  GstFlowReturn ret = GST_FLOW_ERROR;
  GstClockTime buffer_ts = GST_BUFFER_TIMESTAMP (buffer);

  if (GST_CLOCK_TIME_IS_VALID (buffer_ts))
    buffer_ts = self->sync_func (self, buffer, self->user_data);

  if (GST_CLOCK_TIME_IS_VALID (buffer_ts))
    fs_rtp_packet_modder_sync_to_clock (self, buffer_ts);

  buffer = self->modder_func (self, buffer, buffer_ts, self->user_data);

  if (!buffer)
  {
    GST_LOG_OBJECT (self, "Got NULL from FsRtpPacketModderFunc");
    goto invalid;
  }

  ret = gst_pad_push (self->srcpad, buffer);

invalid:

  return ret;
}


static GstCaps *
fs_rtp_packet_modder_getcaps (FsRtpPacketModder *self, GstPad *pad,
  GstCaps *filter)
{
  GstPad *peer;
  GstCaps *caps;
  GstPad *otherpad = self->sinkpad == pad ? self->srcpad : self->sinkpad;

  peer = gst_pad_get_peer (otherpad);

  if (peer)
  {
    GstCaps *peercaps;
    peercaps = gst_pad_query_caps (peer, filter);
    caps = gst_caps_intersect (peercaps, gst_pad_get_pad_template_caps (pad));
    gst_caps_unref (peercaps);
    gst_object_unref (peer);
  }
  else
  {
    caps = gst_caps_intersect (gst_pad_get_pad_template_caps (pad), filter);
  }

  return caps;
}

static gboolean
fs_rtp_packet_modder_sink_event (GstPad *pad, GstObject *parent,
    GstEvent *event)
{
  FsRtpPacketModder *self = FS_RTP_PACKET_MODDER (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      gst_event_copy_segment (event, &self->segment);

      if (self->segment.format != GST_FORMAT_TIME)
        goto newseg_wrong_format;
      break;
    }
    case GST_EVENT_FLUSH_START:
      GST_OBJECT_LOCK (self);
      if (self->clock_id)
      {
        gst_clock_id_unschedule (self->clock_id);
        self->unscheduled = TRUE;
      }
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      break;
  }

  return gst_pad_push_event (self->srcpad, event);

newseg_wrong_format:

  GST_DEBUG_OBJECT (self, "received non TIME segment");
  gst_event_unref (event);
  return FALSE;
}

static GstStateChangeReturn
fs_rtp_packet_modder_change_state (GstElement *element,
    GstStateChange transition)
{
  FsRtpPacketModder *self;
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  self = FS_RTP_PACKET_MODDER (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_OBJECT_LOCK (self);
      /* reset negotiated values */
      self->peer_latency = 0;
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (fs_rtp_packet_modder_parent_class)->change_state (
    element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* we are a live element because we sync to the clock, which we can only
       * do in the PLAYING state */
      if (ret != GST_STATE_CHANGE_FAILURE)
        ret = GST_STATE_CHANGE_NO_PREROLL;
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      GST_OBJECT_LOCK (self);
      if (self->clock_id)
      {
        gst_clock_id_unschedule (self->clock_id);
        self->unscheduled = TRUE;
      }
      GST_OBJECT_UNLOCK (self);
      break;
   default:
      break;
  }

  return ret;
}


static gboolean
fs_rtp_packet_modder_query (GstPad *pad, GstObject *parent, GstQuery *query)
{
  FsRtpPacketModder *self = FS_RTP_PACKET_MODDER (parent);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *caps, *filter;

      gst_query_parse_caps (query, &filter);
      caps = fs_rtp_packet_modder_getcaps (self, pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    case GST_QUERY_LATENCY:
    {
      /* We need to send the query upstream and add the returned latency to our
       * own */
      GstClockTime min_latency, max_latency;
      gboolean us_live;

      if ((res = gst_pad_peer_query (self->sinkpad, query))) {
        gst_query_parse_latency (query, &us_live, &min_latency, &max_latency);

        GST_DEBUG_OBJECT (self, "Peer latency: min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

        /* store this so that we can safely sync on the peer buffers. */
        GST_OBJECT_LOCK (self);
        self->peer_latency = min_latency;
        if (self->clock_id)
          gst_clock_id_unschedule (self->clock_id);
        GST_OBJECT_UNLOCK (self);

        /* we add some latency but can buffer an infinite amount of time */

        GST_DEBUG_OBJECT (self, "Calculated total latency : min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

        gst_query_set_latency (query, TRUE, min_latency, max_latency);
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}
