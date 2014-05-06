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

#ifndef __FS_RTP_XDATA_PAY_H__
#define __FS_RTP_XDATA_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS
#define FS_TYPE_RTP_XDATA_PAY \
  (fs_rtp_xdata_pay_get_type())
#define FS_RTP_XDATA_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),FS_TYPE_RTP_XDATA_PAY,FsRTPXdataPay))
#define FS_RTP_XDATA_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),FS_TYPE_RTP_XDATA_PAY,FsRTPXdataPayClass))
#define FS_IS_RTP_XDATA_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),FS_TYPE_RTP_XDATA_PAY))
#define FS_IS_RTP_XDATA_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),FS_TYPE_RTP_XDATA_PAY))
typedef struct _FsRTPXdataPay FsRTPXdataPay;
typedef struct _FsRTPXdataPayClass FsRTPXdataPayClass;

struct _FsRTPXdataPay
{
  GstRTPBasePayload payload;
};

struct _FsRTPXdataPayClass
{
  GstRTPBasePayloadClass parent_class;
};

GType fs_rtp_xdata_pay_get_type (void);

gboolean fs_rtp_xdata_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __FS_RTP_XDATA_PAY_H__ */
