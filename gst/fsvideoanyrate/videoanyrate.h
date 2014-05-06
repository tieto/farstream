/*
 * Farstream Voice+Video library
 *
 *  Copyright 2008 Collabora Ltd
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

#ifndef __FS_VIDEOANYRATE_H__
#define __FS_VIDEOANYRATE_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

/* #define's don't like whitespacey bits */
#define FS_TYPE_VIDEOANYRATE \
  (fs_videoanyrate_get_type())
#define FS_VIDEOANYRATE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
  FS_TYPE_VIDEOANYRATE,FsVideoanyrate))
#define FS_VIDEOANYRATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
  FS_TYPE_VIDEOANYRATE,FsVideoanyrateClass))
#define FS_IS_VIDEOANYRATE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),FS_TYPE_VIDEOANYRATE))
#define FS_IS_VIDEOANYRATE_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),FS_TYPE_VIDEOANYRATE))

typedef struct _FsVideoanyrate FsVideoanyrate;
typedef struct _FsVideoanyrateClass FsVideoanyrateClass;

struct _FsVideoanyrate
{
  GstBaseTransform parent;
};

struct _FsVideoanyrateClass
{
  GstBaseTransformClass parent_class;
};

GType fs_videoanyrate_get_type (void);

G_END_DECLS

#endif /* __FS_VIDEOANYRATE_H__ */
