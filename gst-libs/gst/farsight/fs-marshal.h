
#ifndef ___fs_marshal_MARSHAL_H__
#define ___fs_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:OBJECT,ENUM,STRING,STRING (./fs-marshal.list:1) */
extern void _fs_marshal_VOID__OBJECT_ENUM_STRING_STRING (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:ENUM,STRING,STRING (./fs-marshal.list:2) */
extern void _fs_marshal_VOID__ENUM_STRING_STRING (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* VOID:BOXED,BOXED (./fs-marshal.list:3) */
extern void _fs_marshal_VOID__BOXED_BOXED (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:OBJECT,OBJECT (./fs-marshal.list:4) */
extern void _fs_marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:UINT,POINTER (./fs-marshal.list:5) */
#define _fs_marshal_VOID__UINT_POINTER	g_cclosure_marshal_VOID__UINT_POINTER

/* VOID:UINT,ENUM (./fs-marshal.list:6) */
extern void _fs_marshal_VOID__UINT_ENUM (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

G_END_DECLS

#endif /* ___fs_marshal_MARSHAL_H__ */

