/*
 * Farstream Voice+Video library
 *
 *  Copyright 2012 Collabora Ltd,
 *   @author: Olivier Crete <olivier.crete@collabora.com>
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

#include "fs-rtp-bin-error-downgrade.h"

GST_DEBUG_CATEGORY (fs_rtp_bin_error_downgrade_debug);
#define GST_CAT_DEFAULT (fs_rtp_bin_error_downgrade_debug)


static void fs_rtp_bin_error_downgrade_handle_message (GstBin * bin,
    GstMessage * message);

G_DEFINE_TYPE (FsRtpBinErrorDowngrade, fs_rtp_bin_error_downgrade, GST_TYPE_BIN);

static void
fs_rtp_bin_error_downgrade_class_init (FsRtpBinErrorDowngradeClass *klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBinClass *gstbin_class = GST_BIN_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (fs_rtp_bin_error_downgrade_debug,
      "fsrtpbinerrordowngrade", 0, "fsrtpbinerrordowngrade");

  gst_element_class_set_metadata (gstelement_class,
      "Farstream Bin Error Downgrader",
      "Bin",
      "Bin that downgrades error messages into warnings",
      "Olivier Crete <olivier.crete@collabora.com>");

  gstbin_class->handle_message = fs_rtp_bin_error_downgrade_handle_message;
}

static void
fs_rtp_bin_error_downgrade_init (FsRtpBinErrorDowngrade *self)
{
}

static void
fs_rtp_bin_error_downgrade_handle_message (GstBin* bin,
    GstMessage* message)
{
  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ERROR)
  {
    GError *error = NULL;
    gchar *debug = NULL;
    gchar *debug2;
    GstMessage *message2;

    gst_message_parse_error (message, &error, &debug);

    debug2 = g_strdup_printf ("FS-WAS-ERROR: %s", debug);

    message2 = gst_message_new_warning (GST_MESSAGE_SRC (message), error,
        debug2);

    g_error_free (error);
    g_free (debug);
    g_free (debug2);
    gst_message_unref (message);

    message = message2;
  }

  GST_BIN_CLASS (fs_rtp_bin_error_downgrade_parent_class)->handle_message (bin,
      message);
}

GstElement *
fs_rtp_bin_error_downgrade_new (const gchar *name)
{
  fs_rtp_bin_error_downgrade_register ();

  return gst_element_factory_make ("fsrtpbinerrordowngrade", name);
}


void
fs_rtp_bin_error_downgrade_register (void)
{
  static gsize initialization_value = 0;

  if (g_once_init_enter (&initialization_value))
  {
    gsize setup_value = gst_element_register (NULL, "fsrtpbinerrordowngrade",
        GST_RANK_MARGINAL, FS_TYPE_RTP_BIN_ERROR_DOWNGRADE);

    g_once_init_leave (&initialization_value, setup_value);
  }
}

