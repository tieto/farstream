/*
 * Farstream Voice+Video library
 *
 *  Copyright 2008 Collabora Ltd,
 *  Copyright 2008 Nokia Corporation
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __CHECK_THREADSAFE_H__
#define __CHECK_THREADSAFE_H__

#include <gst/check/gstcheck.h>

/* Define thread safe versions of the tests */

#define ts_fail_unless(...)             \
  G_STMT_START {                        \
    g_mutex_lock (&check_mutex);         \
    fail_unless (__VA_ARGS__);          \
    g_mutex_unlock (&check_mutex);       \
  } G_STMT_END


#define ts_fail_if(...)                 \
  G_STMT_START {                        \
    g_mutex_lock (&check_mutex);         \
    fail_if (__VA_ARGS__);              \
    g_mutex_unlock (&check_mutex);       \
  } G_STMT_END


#define ts_fail(...)    \
  G_STMT_START {                        \
    g_mutex_lock (&check_mutex);         \
    fail (__VA_ARGS__);                 \
    g_mutex_unlock (&check_mutex);       \
  } G_STMT_END

#endif /* __CHECK_THREADSAFE_H__ */
