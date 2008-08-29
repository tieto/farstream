/*
 * Farsight2 - Farsight UPnP IGD abstraction
 *
 * Copyright 2008 Collabora Ltd.
 *  @author: Olivier Crete <olivier.crete@collabora.co.uk>
 * Copyright 2008 Nokia Corp.
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


#include "fs-upnp-simple-igd.h"

#include <libgupnp/gupnp-control-point.h>


struct _FsUpnpSimpleIgdPrivate
{
  GMainContext *main_context;

  GUPnPContext *gupnp_context;
  GUPnPControlPoint *cp;

  GPtrArray *service_proxies;

  gulong avail_handler;
  gulong unavail_handler;

  guint request_timeout;

  gboolean gathering;
};

/* signals */
enum
{
  SIGNAL_NEW_EXTERNAL_IP,
  SIGNAL_ERROR,
  LAST_SIGNAL
};

/* props */
enum
{
  PROP_0,
  PROP_REQUEST_TIMEOUT
};


static guint signals[LAST_SIGNAL] = { 0 };


#define FS_UPNP_SIMPLE_IGD_GET_PRIVATE(o)                                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), FS_TYPE_UPNP_SIMPLE_IGD,             \
   FsUpnpSimpleIgdPrivate))


G_DEFINE_TYPE (FsUpnpSimpleIgd, fs_upnp_simple_igd, G_TYPE_OBJECT);


static void fs_upnp_simple_igd_dispose (GObject *object);
static void fs_upnp_simple_igd_finalize (GObject *object);
static void fs_upnp_simple_igd_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);
static void fs_upnp_simple_igd_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec);

static void fs_upnp_simple_igd_gather_proxy (FsUpnpSimpleIgd *self,
    GUPnPServiceProxy *proxy);

static void
fs_upnp_simple_igd_class_init (FsUpnpSimpleIgdClass *klass)
{
 GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = fs_upnp_simple_igd_dispose;
  gobject_class->finalize = fs_upnp_simple_igd_finalize;
  gobject_class->set_property = fs_upnp_simple_igd_set_property;
  gobject_class->get_property = fs_upnp_simple_igd_get_property;

  g_object_class_install_property (gobject_class,
      PROP_REQUEST_TIMEOUT,
      g_param_spec_int ("request-timeout",
          "The timeout after which a request is considered to have failed",
          "After this timeout, the request is considered to have failed and"
          "is dropped.",
          0, G_MAXUINT, 5,
          G_PARAM_READWRITE));

  /**
   * FsUpnpSimpleIgd::new-external-ip
   * @self: #FsUpnpSimpleIgd that emitted the signal
   * @ip: The string representing the new external IP
   *
   * This signal means that a new external IP has been found on an IGD.
   * It is only emitted if fs_upnp_simple_igd_gather() has been set to %TRUE.
   *
   */
  signals[SIGNAL_NEW_EXTERNAL_IP] = g_signal_new ("new-external-ip",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL,
      NULL,
      g_cclosure_marshal_VOID__STRING,
      G_TYPE_NONE, 1, G_TYPE_STRING);

  /**
   * FsUpnpSimpleIgd::error
   * @self: #FsUpnpSimpleIgd that emitted the signal
   * @error: a #GError
   *
   * This means that an asynchronous error has happened.
   *
   */
  signals[SIGNAL_ERROR] = g_signal_new ("error",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL,
      NULL,
      g_cclosure_marshal_VOID__POINTER,
      G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
fs_upnp_simple_igd_init (FsUpnpSimpleIgd *self)
{
  self->priv = FS_UPNP_SIMPLE_IGD_GET_PRIVATE (self);

  self->priv->request_timeout = 5;

  self->priv->service_proxies = g_ptr_array_new ();
}

static void
fs_upnp_simple_igd_dispose (GObject *object)
{
  FsUpnpSimpleIgd *self = FS_UPNP_SIMPLE_IGD_CAST (object);

  if (self->priv->avail_handler)
    g_signal_handler_disconnect (self->priv->cp, self->priv->avail_handler);
  self->priv->avail_handler = 0;

  if (self->priv->unavail_handler)
    g_signal_handler_disconnect (self->priv->cp, self->priv->unavail_handler);
  self->priv->unavail_handler = 0;

  while (g_ptr_array_index(self->priv->service_proxies, 0))
    g_object_unref ( G_OBJECT (
            g_ptr_array_remove_index_fast (self->priv->service_proxies, 0)));

  if (self->priv->cp)
    g_object_unref (self->priv->cp);
  self->priv->cp = NULL;

  if (self->priv->gupnp_context)
    g_object_unref (self->priv->gupnp_context);
  self->priv->gupnp_context = NULL;

  G_OBJECT_CLASS (fs_upnp_simple_igd_parent_class)->dispose (object);
}

static void
fs_upnp_simple_igd_finalize (GObject *object)
{
  FsUpnpSimpleIgd *self = FS_UPNP_SIMPLE_IGD_CAST (object);

  g_main_context_unref (self->priv->main_context);

  g_ptr_array_free (self->priv->service_proxies, TRUE);

  G_OBJECT_CLASS (fs_upnp_simple_igd_parent_class)->finalize (object);
}

static void
fs_upnp_simple_igd_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
  FsUpnpSimpleIgd *self = FS_UPNP_SIMPLE_IGD_CAST (object);

  switch (prop_id) {
    case PROP_REQUEST_TIMEOUT:
      g_value_set_uint (value, self->priv->request_timeout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}

static void
fs_upnp_simple_igd_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
  FsUpnpSimpleIgd *self = FS_UPNP_SIMPLE_IGD_CAST (object);

  switch (prop_id) {
    case PROP_REQUEST_TIMEOUT:
      self->priv->request_timeout = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
_cp_service_avail (GUPnPControlPoint *cp,
    GUPnPServiceProxy *proxy,
    FsUpnpSimpleIgd *self)
{
  g_ptr_array_add (self->priv->service_proxies, g_object_ref (proxy));

  if (self->priv->gathering)
    fs_upnp_simple_igd_gather_proxy (self, proxy);
}


static void
_cp_service_unavail (GUPnPControlPoint *cp,
    GUPnPServiceProxy *proxy,
    FsUpnpSimpleIgd *self)
{
  if (g_ptr_array_remove(self->priv->service_proxies, proxy))
    g_object_unref (proxy);
}


static gboolean
fs_upnp_simple_igd_build (FsUpnpSimpleIgd *self, GError **error)
{
  self->priv->gupnp_context = gupnp_context_new (self->priv->main_context,
      NULL, 0, error);
  if (!self->priv->gupnp_context)
    return FALSE;

  self->priv->cp = gupnp_control_point_new (self->priv->gupnp_context,
      "urn:schemas-upnp-org:service:WANIPConnection:1");
  g_return_val_if_fail (self->priv->cp, FALSE);

  self->priv->avail_handler = g_signal_connect (self->priv->cp,
      "service-proxy-available",
      G_CALLBACK (_cp_service_avail), self);
  self->priv->unavail_handler = g_signal_connect (self->priv->cp,
      "service-proxy-unavailable",
      G_CALLBACK (_cp_service_unavail), self);

  gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (self->priv->cp),
      TRUE);

  return TRUE;
}

FsUpnpSimpleIgd *
fs_upnp_simple_igd_new (GMainContext *main_context)
{
  FsUpnpSimpleIgd *self = g_object_new (FS_TYPE_UPNP_SIMPLE_IGD, NULL);

  self->priv->main_context = g_main_context_ref (main_context);

  fs_upnp_simple_igd_build (self, NULL);

  return self;
}

void
fs_upnp_simple_igd_gather (FsUpnpSimpleIgd *self, gboolean gather)
{
  if (self->priv->gathering == gather)
    return;

  self->priv->gathering = gather;

  if (gather)
  {
    gint i;

    for (i = 0; g_ptr_array_index(self->priv->service_proxies, i); i++)
    {
      GUPnPServiceProxy *proxy =
        g_ptr_array_index(self->priv->service_proxies, i);
      fs_upnp_simple_igd_gather_proxy (self, proxy);
    }
  }
}


static void
_service_proxy_got_external_ip_address (GUPnPServiceProxy *proxy,
    GUPnPServiceProxyAction *action,
    gpointer user_data)
{
  GError *error = NULL;
  gchar *ip = NULL;

  gupnp_service_proxy_end_action (proxy, action, &error,
      "NewExternalIPAddress", G_TYPE_STRING, &ip,
      NULL);
}

static void
fs_upnp_simple_igd_gather_proxy (FsUpnpSimpleIgd *self,
    GUPnPServiceProxy *proxy)
{
  GUPnPServiceProxyAction *action;

  action = gupnp_service_proxy_begin_action (proxy, "GetExternalIPAddress",
      _service_proxy_got_external_ip_address,
      self,
      NULL);
}
