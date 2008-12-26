
#ifndef ___fs_marshal_MARSHAL_H__
#define ___fs_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:OBJECT,INT,STRING,STRING (./fs-marshal.list:1) */
extern void _fs_marshal_VOID__OBJECT_INT_STRING_STRING (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:INT,STRING,STRING (./fs-marshal.list:2) */
extern void _fs_marshal_VOID__INT_STRING_STRING (GClosure     *closure,
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

G_END_DECLS

#endif /* ___fs_marshal_MARSHAL_H__ */

