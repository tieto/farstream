/*
 * Farsight2 - GStreamer interfaces
 *
 * Copyright 2007 Collabora Ltd.
 *  @author: Philippe Kalaf <philippe.kalaf@collabora.co.uk>
 * Copyright 2007 Nokia Corp.
 *
 * fs-conference.h - Header file for gstreamer interface to be implemented by
 *                   farsight conference elements
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FS_CONFERENCE_H__
#define __FS_CONFERENCE_H__

#include <gst/gst.h>
#include <gst/interfaces/interfaces-enumtypes.h>

#include <gst/farsight/fs-session.h>
#include <gst/farsight/fs-codec.h>

G_BEGIN_DECLS

#define FS_TYPE_CONFERENCE \
  (fs_conference_get_type ())
#define FS_CONFERENCE(obj) \
  (GST_IMPLEMENTS_INTERFACE_CHECK_INSTANCE_CAST ((obj), FS_TYPE_CONFERENCE, FsConference))
#define FS_IS_CONFERENCE(obj) \
  (GST_IMPLEMENTS_INTERFACE_CHECK_INSTANCE_TYPE ((obj), FS_TYPE_CONFERENCE))
#define FS_CONFERENCE_GET_IFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), FS_TYPE_CONFERENCE, FsConferenceInterface))

/**
 * FsConference:
 *
 * Opaque #FsConference data structure.
 */
typedef struct _FsConference FsConference;

typedef struct _FsConferenceInterface FsConferenceInterface;

/**
 * FsConferenceInterface:
 * @parent: parent interface type.
 * @new_session: virtual method to create a new conference session
 * @new_participant: virtual method to create a new participant
 *
 * #FsConferenceInterface interface.
 */
struct _FsConferenceInterface {
  GTypeInterface parent;

  /* virtual functions */
  FsSession *(* new_session) (FsConference *conference, FsMediaType media_type,
                              GError **error);

  FsParticipant *(* new_participant) (FsConference *conference,
                                      gchar *cname);

  /*< private > */
  gpointer _gst_reserved[GST_PADDING];
};

GType fs_conference_get_type (void);

/* virtual class function wrappers */
FsSession *fs_conference_new_session (FsConference *conference,
                                      FsMediaType media_type,
                                      GError **error);

FsParticipant *fs_conference_new_participant (FsConference *conference,
                                              gchar *cname);

G_END_DECLS

#endif /* __FS_CONFERENCE_H__ */
