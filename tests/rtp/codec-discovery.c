/* Farstream ad-hoc test for the rtp codec discovery
 *
 * Copyright (C) 2007 Collabora, Nokia
 * @author: Olivier Crete <olivier.crete@collabora.co.uk>
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

#include <gst/gst.h>

#include <farstream/fs-codec.h>

#include "fs-rtp-discover-codecs.h"
#include "fs-rtp-conference.h"


static void
debug_pipeline (const gchar *prefix, GList *pipeline)
{
  GList *walk;
  GString *str;
  gboolean first = FALSE;

  str = g_string_new (prefix);

  for (walk = pipeline; walk; walk = g_list_next (walk))
  {
    GList *walk2;
    gboolean first_alt = TRUE;

    if (!first)
      g_string_append (str, " ->");
    first = FALSE;

    for (walk2 = g_list_first (walk->data); walk2; walk2 = g_list_next (walk2))
    {
      if (first_alt)
        g_string_append_printf (str, " %s",
            gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (walk2->data)));
      else
        g_string_append_printf (str, " | %s",
            gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (walk2->data)));

      first_alt = FALSE;
    }
  }
  g_print ("%s\n", str->str);
  g_string_free (str, TRUE);
}

static void
debug_blueprint (CodecBlueprint *blueprint)
{
  gchar *str;

  str = fs_codec_to_string (blueprint->codec);
  g_print ("Codec: %s\n", str);
  g_free (str);

  str = gst_caps_to_string (blueprint->media_caps);
  g_print ("media_caps: %s\n", str);
  g_free (str);

  str = gst_caps_to_string (blueprint->rtp_caps);
  g_print ("rtp_caps: %s\n", str);
  g_free (str);

  str = gst_caps_to_string (blueprint->input_caps);
  g_print ("input_caps: %s\n", str);
  g_free (str);

  str = gst_caps_to_string (blueprint->output_caps);
  g_print ("output_caps: %s\n", str);
  g_free (str);

  debug_pipeline ("send pipeline:", blueprint->send_pipeline_factory);

  debug_pipeline ("recv pipeline:", blueprint->receive_pipeline_factory);

  g_print ("================================\n");
}

int main (int argc, char **argv)
{
  GList *elements = NULL;
  GError *error = NULL;

  gst_init (&argc, &argv);

  GST_DEBUG_CATEGORY_INIT (fsrtpconference_debug, "fsrtpconference", 0,
      "Farstream RTP Conference Element");
  GST_DEBUG_CATEGORY_INIT (fsrtpconference_disco, "fsrtpconference_disco",
      0, "Farstream RTP Codec Discovery");
  GST_DEBUG_CATEGORY_INIT (fsrtpconference_nego, "fsrtpconference_nego",
      0, "Farstream RTP Codec Negotiation");

  gst_debug_set_default_threshold (GST_LEVEL_WARNING);

  g_print ("AUDIO STARTING!!\n");

  elements = fs_rtp_blueprints_get (FS_MEDIA_TYPE_AUDIO, &error);

  if (error)
    g_printerr ("Error: %s\n", error->message);
  else
    g_list_foreach (elements, (GFunc) debug_blueprint, NULL);

  g_clear_error (&error);

  fs_rtp_blueprints_unref (FS_MEDIA_TYPE_AUDIO);

  g_print ("AUDIO FINISHED!!\n");


  g_print ("VIDEO STARTING!!\n");

  elements = fs_rtp_blueprints_get (FS_MEDIA_TYPE_VIDEO, &error);

  if (error)
    g_printerr ("Error: %s\n", error->message);
  else
    g_list_foreach (elements, (GFunc) debug_blueprint, NULL);

  g_clear_error (&error);

  fs_rtp_blueprints_unref (FS_MEDIA_TYPE_VIDEO);

  g_print ("VIDEO FINISHED!!\n");

  return 0;
}
