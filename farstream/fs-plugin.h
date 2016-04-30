/*
 * fs-plugin.h - Header for farstream plugin infrastructure
 *
 * Farstream Voice+Video library
 * Copyright (c) 2005 INdT.
 *   @author: Andre Moreira Magalhaes <andre.magalhaes@indt.org.br>
 * Copyright (c) 2005-2007 Collabora Ltd.
 * Copyright (c) 2005-2007 Nokia Corp.
 *   @author: Rob Taylor <rob.taylor@collabora.co.uk>
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

#ifndef __FS_PLUGIN_H__
#define __FS_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gst/gst.h>

#include <stdarg.h>

G_BEGIN_DECLS


/* TYPE MACROS */
#define FS_TYPE_PLUGIN \
  (fs_plugin_get_type ())
#define FS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), FS_TYPE_PLUGIN, FsPlugin))
#define FS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), FS_TYPE_PLUGIN, FsPluginClass))
#define FS_IS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), FS_TYPE_PLUGIN))
#define FS_IS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), FS_TYPE_PLUGIN))
#define FS_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FS_TYPE_PLUGIN, FsPluginClass))

/**
 * FsPlugin:
 * @parent: the parent object
 *
 * This structure represents a plugin, it is opaque.
 */

typedef struct _FsPlugin FsPlugin;
typedef struct _FsPluginClass FsPluginClass;
typedef struct _FsPluginPrivate FsPluginPrivate;

struct _FsPlugin
{
  GTypeModule parent;

  /*< private >*/

  GType  type;

  gchar *name;                  /* name of the plugin */

  /*< private >*/

  FsPluginPrivate *priv;

  gpointer unused[8];
};

struct _FsPluginClass
{
  GTypeModuleClass parent_class;

  /*< private >*/

  gpointer unused[8];
};

GType fs_plugin_get_type (void);


GObject *fs_plugin_create_valist (const gchar *name,
                                  const gchar *type_suffix,
                                  GError **error,
                                  const gchar *first_property_name,
                                  va_list var_args);

GObject *fs_plugin_create (const gchar *name,
                           const gchar *type_suffix,
                           GError **error,
                           const gchar *first_property_name, ...);

gchar **fs_plugin_list_available (const gchar *type_suffix);

void fs_plugin_register_static (const gchar *name, const gchar *type_suffix,
        GType type);

/**
 * FS_INIT_PLUGIN:
 * @type_register_func: A function that register a #GType and returns it
 *
 * This macro is used to declare Farstream plugins and must be used once
 * in any farstream plugin.
 */

#define _FS_REGISTER_TYPE(plugin,name,type) \
    fs_ ## name ## _ ## type ## _register_type (plugin)

#ifdef GST_PLUGIN_BUILD_STATIC

#define FS_INIT_PLUGIN(name,type)                       \
    G_MODULE_EXPORT void                                \
    fs_plugin_ ## name ## _ ## type ## _register (void) \
    {                                                   \
      fs_plugin_register_static (#name, #type,          \
          _FS_REGISTER_TYPE (NULL, name, type));        \
    }

#else /* !GST_PLUGIN_BUILD_STATIC */

#define FS_INIT_PLUGIN(name,_type)                            \
    G_MODULE_EXPORT void                                      \
    fs_init_plugin (FsPlugin *plugin)                         \
    {                                                         \
      plugin->type = _FS_REGISTER_TYPE (plugin, name, _type); \
    }

#endif /* GST_PLUGIN_BUILD_STATIC */

/**
 * FS_PLUGIN_STATIC_DECLARE:
 * @name: unique name of the plugin
 *
 * This macro can be used to initialize statically linked plugins. It is
 * necessary to call this macro before the plugin can be used. It has to be
 * used in combination with FS_PLUGIN_STATIC_REGISTER and must be placed
 * outside any block to declare the plugin initialization function.
 */
#define FS_PLUGIN_STATIC_DECLARE(name) \
    extern void fs_plugin_ ## name ## _register(void);

/**
 * FS_PLUGIN_STATIC_REGISTER:
 * @name: unique name of the plugin
 *
 * This macro can be used to initialize statically linked plugins. It is
 * necessary to call this macro before the plugin can be used. It has to
 * be used in combination with FS_PLUGIN_STATIC_DECLARE and calls the plugin
 * initialization function.
 */
#define FS_PLUGIN_STATIC_REGISTER(name) fs_plugin_ ## name ## _register ()

G_END_DECLS
#endif

