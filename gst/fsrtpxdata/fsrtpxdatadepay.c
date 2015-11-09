/*
 * Microsoft Lync RTP x-data Depayloader Gst Element
 *
 * Copyright 2014 Collabora Ltd.
 *   @author: Olivier Crete <olivier.crete@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write tox the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>
#include "fsrtpxdatadepay.h"

static GstStaticPadTemplate fs_rtp_xdata_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"application\", "
        "encoding-name = (string) \"X-DATA\"")
    );

static GstStaticPadTemplate fs_rtp_xdata_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/octet-stream")
    );

static GstBuffer *fs_rtp_xdata_depay_process (GstRTPBaseDepayload * depayload,
    GstBuffer * buf);

G_DEFINE_TYPE (FsRTPXdataDepay, fs_rtp_xdata_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
fs_rtp_xdata_depay_class_init (FsRTPXdataDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->process = fs_rtp_xdata_depay_process;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_xdata_depay_src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_xdata_depay_sink_template));
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Depayloader for Microsoft Lync x-data",
      "Codec/Depayloader/Network/RTP",
      "Extracts data from Microsoft Lync RTP x-data packets",
      "Olivier Crete <olivier.crete@collabora.com>");
}

static void
fs_rtp_xdata_depay_init (FsRTPXdataDepay * rtpxdatadepay)
{
  GstRTPBaseDepayload *depayload = GST_RTP_BASE_DEPAYLOAD (rtpxdatadepay);

  depayload->clock_rate = 90000;
}

static GstBuffer *
fs_rtp_xdata_depay_process (GstRTPBaseDepayload * depayload, GstBuffer * buf)
{
  GstBuffer *outbuf;
  GstRTPBuffer rtp = { NULL };

  gst_rtp_buffer_map (buf, GST_MAP_READ, &rtp);
  outbuf = gst_rtp_buffer_get_payload_buffer (&rtp);
  gst_rtp_buffer_unmap (&rtp);

  return outbuf;
}

gboolean
fs_rtp_xdata_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "fsrtpxdatadepay",
      GST_RANK_SECONDARY, FS_TYPE_RTP_XDATA_DEPAY);
}
