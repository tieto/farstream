
#ifndef ___fs_rawudp_marshal_MARSHAL_H__
#define ___fs_rawudp_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:BOXED,BOXED (./fs-rawudp-marshal.list:1) */
extern void _fs_rawudp_marshal_VOID__BOXED_BOXED (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* VOID:ENUM,STRING,STRING (./fs-rawudp-marshal.list:2) */
extern void _fs_rawudp_marshal_VOID__ENUM_STRING_STRING (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:UINT,POINTER (./fs-rawudp-marshal.list:3) */
#define _fs_rawudp_marshal_VOID__UINT_POINTER	g_cclosure_marshal_VOID__UINT_POINTER

G_END_DECLS

#endif /* ___fs_rawudp_marshal_MARSHAL_H__ */

