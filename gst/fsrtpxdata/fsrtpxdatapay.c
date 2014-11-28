/*
 * Microsoft Lync RTP x-data Payloader Gst Element
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsrtpxdatapay.h"
#include <gst/rtp/gstrtpbuffer.h>

GST_DEBUG_CATEGORY_STATIC (rtpxdatapay_debug);
#define GST_CAT_DEFAULT (rtpxdatapay_debug)

static GstStaticPadTemplate fs_rtp_xdata_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/octet-stream")
    );

static GstStaticPadTemplate fs_rtp_xdata_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"application\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, "
        "encoding-name = (string) \"X-DATA\"")
    );

#define MAX_PAYLOAD_SIZE 1200

static gboolean fs_rtp_xdata_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn fs_rtp_xdata_pay_handle_buffer (GstRTPBasePayload *payload,
    GstBuffer *buffer);

G_DEFINE_TYPE (FsRTPXdataPay, fs_rtp_xdata_pay,
    GST_TYPE_RTP_BASE_PAYLOAD);

static void
fs_rtp_xdata_pay_class_init (FsRTPXdataPayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gstrtpbasepayload_class->set_caps = fs_rtp_xdata_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = fs_rtp_xdata_pay_handle_buffer;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_xdata_pay_sink_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fs_rtp_xdata_pay_src_template));
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Payloader for Microsoft Lync x-data", "Codec/Payloader/Network/RTP",
      "Packetize Microsoft Lync x-data streams into RTP packets",
      "Olivier Crete <olivier.crete@collabora.com>");

  GST_DEBUG_CATEGORY_INIT (rtpxdatapay_debug, "fsrtpxdatapay", 0,
      "Microsoft Lync x-data RTP payloader");
}

static void
fs_rtp_xdata_pay_init (FsRTPXdataPay * rtpxdatapay)
{
  GstRTPBasePayload *rtpbasepayload;

  rtpbasepayload = GST_RTP_BASE_PAYLOAD (rtpxdatapay);

  gst_rtp_base_payload_set_options (rtpbasepayload, "application", TRUE,
      "X-DATA", 90000);
  GST_RTP_BASE_PAYLOAD_MTU(rtpbasepayload) = MAX_PAYLOAD_SIZE +
      gst_rtp_buffer_calc_header_len (0);
}

static gboolean
fs_rtp_xdata_pay_setcaps (GstRTPBasePayload * rtpbasepayload, GstCaps * caps)
{
  return gst_rtp_base_payload_set_outcaps (rtpbasepayload, NULL);
}

static GstFlowReturn
fs_rtp_xdata_pay_handle_buffer (GstRTPBasePayload *payload, GstBuffer *buffer)
{
  GstBuffer *rtpbuf;
  gsize size;
  guint mtu;

  size = gst_buffer_get_size (buffer);
  mtu = GST_RTP_BASE_PAYLOAD_MTU(payload);
  mtu -= gst_rtp_buffer_calc_header_len (0);

  if (size <= mtu) {
    rtpbuf = gst_rtp_buffer_new_allocate (0, 0, 0);
    rtpbuf = gst_buffer_append (rtpbuf, buffer);

    return gst_rtp_base_payload_push (payload, rtpbuf);
  } else {
    GstBufferList *rtplist = gst_buffer_list_new_sized (2);
    gsize offset = 0;
    gsize new_size;

    while (size > 0) {
      new_size = size > mtu ? mtu : size;

      rtpbuf = gst_rtp_buffer_new_allocate (0, 0, 0);
      rtpbuf = gst_buffer_append_region (rtpbuf, buffer, offset, new_size);
      gst_buffer_list_add (rtplist, rtpbuf);
      offset += new_size;
      size -= new_size;
    }
    return gst_rtp_base_payload_push_list (payload, rtplist);
  }
}

gboolean
fs_rtp_xdata_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "fsrtpxdatapay",
      GST_RANK_SECONDARY, FS_TYPE_RTP_XDATA_PAY);
}
