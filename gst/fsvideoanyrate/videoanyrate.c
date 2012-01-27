/*
 * Farstream Voice+Video library
 *
 *  Copyright 2007-2012 Collabora Ltd, 
 *  Copyright 2007 Nokia Corporation
 *   @author: Olivier Crete <olivier.crete@collabora.co.uk>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * SECTION:element-fsvideoanyrate
 * @short_description: Removes the framerate from video caps
 *
 * This element will remove the framerate from video caps, it is a poor man's
 * videorate for live pipelines.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "videoanyrate.h"

#include <string.h>

GST_DEBUG_CATEGORY (videoanyrate_debug);
#define GST_CAT_DEFAULT (videoanyrate_debug)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* Videoanyrate signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0,
};


static GstCaps *
gst_videoanyrate_transform_caps (GstBaseTransform *trans,
    GstPadDirection direction,
    GstCaps *caps,
    GstCaps *filter);
static void
gst_videoanyrate_fixate_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);


G_DEFINE_TYPE (GstVideoanyrate, gst_videoanyrate, GST_TYPE_BASE_TRANSFORM);


static void
gst_videoanyrate_class_init (GstVideoanyrateClass *klass)
{
  GstElementClass *element_class;
  GstBaseTransformClass *gstbasetransform_class;

  element_class = GST_ELEMENT_CLASS (klass);
  gstbasetransform_class = GST_BASE_TRANSFORM_CLASS (klass);


  GST_DEBUG_CATEGORY_INIT
    (videoanyrate_debug, "fsvideoanyrate", 0, "fsvideoanyrate");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_metadata (element_class,
      "Videoanyrate element",
      "Filter",
      "This element removes the framerate from caps",
      "Olivier Crete <olivier.crete@collabora.com>");

  gstbasetransform_class->transform_caps =
    GST_DEBUG_FUNCPTR(gst_videoanyrate_transform_caps);
  gstbasetransform_class->fixate_caps =
    GST_DEBUG_FUNCPTR(gst_videoanyrate_fixate_caps);
}

static void
gst_videoanyrate_init (GstVideoanyrate *videoanyrate)
{
}

static GstCaps *
gst_videoanyrate_transform_caps (GstBaseTransform *trans,
    GstPadDirection direction,
    GstCaps *caps,
    GstCaps *filter)
{
  GstCaps *mycaps = gst_caps_copy (caps);
  guint i;

  if (gst_caps_get_size (mycaps) == 0)
    return mycaps;

  GST_DEBUG_OBJECT (trans, "Transforming caps");

  for (i = 0; i < gst_caps_get_size (mycaps); i++)
  {
    GstStructure *s;

    s = gst_caps_get_structure (mycaps, i);

    if (gst_structure_has_field (s, "framerate"))
      gst_structure_set (s,
          "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
  }

  return mycaps;
}

static void
gst_videoanyrate_fixate_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps)
{
  GstStructure *ins, *outs;

  const GValue *from_fr, *to_fr;

  g_return_if_fail (gst_caps_is_fixed (caps));

  GST_DEBUG_OBJECT (base, "trying to fixate othercaps %" GST_PTR_FORMAT
      " based on caps %" GST_PTR_FORMAT, othercaps, caps);

  ins = gst_caps_get_structure (caps, 0);
  outs = gst_caps_get_structure (othercaps, 0);

  from_fr = gst_structure_get_value (ins, "framerate");
  to_fr = gst_structure_get_value (outs, "framerate");

  /* we have both PAR but they might not be fixated */
  if (from_fr && to_fr && !gst_value_is_fixed (to_fr)) {
    gint from_fr_n, from_fr_d;

    /* from_fr should be fixed */
    g_return_if_fail (gst_value_is_fixed (from_fr));

    from_fr_n = gst_value_get_fraction_numerator (from_fr);
    from_fr_d = gst_value_get_fraction_denominator (from_fr);

    GST_DEBUG_OBJECT (base, "fixating to_fr nearest to %d/%d",
        from_fr_n, from_fr_d);
    gst_structure_fixate_field_nearest_fraction (outs, "framerate",
        from_fr_n, from_fr_d);
  }
}
gboolean
gst_videoanyrate_plugin_init (GstPlugin *plugin)
{
  return gst_element_register (plugin, "fsvideoanyrate",
      GST_RANK_MARGINAL, GST_TYPE_VIDEOANYRATE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "fsvideoanyrate",
    "Videoanyrate",
    gst_videoanyrate_plugin_init, VERSION, "LGPL", "Farstream",
    "http://farstream.sf.net")
