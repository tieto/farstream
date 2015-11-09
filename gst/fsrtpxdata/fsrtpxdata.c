/*
 * Microsoft Lync RTP x-data Gst Element
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

#include "fsrtpxdatapay.h"
#include "fsrtpxdatadepay.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!fs_rtp_xdata_depay_plugin_init (plugin))
    return FALSE;

  if (!fs_rtp_xdata_pay_plugin_init (plugin))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    fsrtpxdata,
    "Farstream Microsoft Lync x-data RTP plugin",
    plugin_init, VERSION, "LGPL", "Farstream",
    "http://www.freedesktop.org/wiki/Software/Farstream")
