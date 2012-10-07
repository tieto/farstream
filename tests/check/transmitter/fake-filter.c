/*
 * Farstream Voice+Video library
 *
 *  Copyright 2008-2012 Collabora Ltd,
 *  Copyright 2008 Nokia Corporation
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

#include "fake-filter.h"

GST_DEBUG_CATEGORY (fake_filter_debug);
#define GST_CAT_DEFAULT (fake_filter_debug)


static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void fs_fake_filter_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec);
static void fs_fake_filter_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec);

/* signals and args */

enum
{
  PROP_0,
  PROP_SENDING
};

G_DEFINE_TYPE (FsFakeFilter, fs_fake_filter, GST_TYPE_BASE_TRANSFORM);


static void
fs_fake_filter_class_init (FsFakeFilterClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT
    (fake_filter_debug, "fsfakefilter", 0, "fsfakefilter");

  gobject_class->set_property = fs_fake_filter_set_property;
  gobject_class->get_property = fs_fake_filter_get_property;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_metadata (gstelement_class,
      "Fake Filter element",
      "Filter",
      "This element ignores the sending property",
      "Olivier Crete <olivier.crete@collabora.com>");

  g_object_class_install_property (gobject_class,
      PROP_SENDING,
      g_param_spec_boolean ("sending",
          "Sending RTP?",
          "If set to FALSE, it assumes that all RTP has been dropped",
          FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
fs_fake_filter_init (FsFakeFilter *fakefilter)
{
}

static void
fs_fake_filter_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_SENDING:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
static void
fs_fake_filter_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_SENDING:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


gboolean
fs_fake_filter_plugin_init (GstPlugin *plugin)
{
  return gst_element_register (plugin, "fsfakefilter",
      GST_RANK_MARGINAL, FS_TYPE_FAKE_FILTER);
}

gboolean
fs_fake_filter_register (void)
{
  return gst_plugin_register_static (
      GST_VERSION_MAJOR,
      GST_VERSION_MINOR,
      "fsfakefilter",
      "FakeFilter",
      fs_fake_filter_plugin_init,
      VERSION,
      "LGPL",
      "Farstream",
      "Farstream",
      "Farstream testing suite");
}
