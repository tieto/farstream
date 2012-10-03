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

#ifndef __FS_RTP_BIN_ERROR_DOWNGRADE_H__
#define __FS_RTP_BIN_ERROR_DOWNGRADE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #define's don't like whitespacey bits */
#define FS_TYPE_RTP_BIN_ERROR_DOWNGRADE \
  (fs_rtp_bin_error_downgrade_get_type())
#define FS_RTP_BIN_ERROR_DOWNGRADE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
  FS_TYPE_RTP_BIN_ERROR_DOWNGRADE,FsRtpBinErrorDowngrade))
#define FS_RTP_BIN_ERROR_DOWNGRADE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
  FS_TYPE_RTP_BIN_ERROR_DOWNGRADE,FsRtpBinErrorDowngradeClass))
#define FS_IS_RTP_BIN_ERROR_DOWNGRADE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),FS_TYPE_RTP_BIN_ERROR_DOWNGRADE))
#define FS_IS_RTP_BIN_ERROR_DOWNGRADE_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),FS_TYPE_RTP_BIN_ERROR_DOWNGRADE))

typedef struct _FsRtpBinErrorDowngrade FsRtpBinErrorDowngrade;
typedef struct _FsRtpBinErrorDowngradeClass FsRtpBinErrorDowngradeClass;
typedef struct _FsRtpBinErrorDowngradePrivate FsRtpBinErrorDowngradePrivate;

struct _FsRtpBinErrorDowngrade
{
  GstBin parent;
};

struct _FsRtpBinErrorDowngradeClass
{
  GstBinClass parent_class;
};

GType fs_rtp_bin_error_downgrade_get_type (void);

void fs_rtp_bin_error_downgrade_register (void);

GstElement *fs_rtp_bin_error_downgrade_new (const gchar *name);

G_END_DECLS

#endif /* __FS_RTP_BIN_ERROR_DOWNGRADE_H__ */
