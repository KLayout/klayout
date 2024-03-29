/**
 *  Common header for Qt binding definition library
 *
 *  DO NOT EDIT THIS FILE. 
 *  This file has been created automatically
 */

#include "tlDefs.h"

#if !defined(HDR_gsiQtCore5CompatCommon_h)
# define HDR_gsiQtCore5CompatCommon_h

# ifdef MAKE_GSI_QTCORE5COMPAT_LIBRARY
#   define GSI_QTCORE5COMPAT_PUBLIC           DEF_INSIDE_PUBLIC
#   define GSI_QTCORE5COMPAT_PUBLIC_TEMPLATE  DEF_INSIDE_PUBLIC_TEMPLATE
#   define GSI_QTCORE5COMPAT_LOCAL            DEF_INSIDE_LOCAL
# else
#   define GSI_QTCORE5COMPAT_PUBLIC           DEF_OUTSIDE_PUBLIC
#   define GSI_QTCORE5COMPAT_PUBLIC_TEMPLATE  DEF_OUTSIDE_PUBLIC_TEMPLATE
#   define GSI_QTCORE5COMPAT_LOCAL            DEF_OUTSIDE_LOCAL
# endif

#define FORCE_LINK_GSI_QTCORE5COMPAT GSI_QTCORE5COMPAT_PUBLIC int _force_link_gsiQtCore5Compat_f (); int _force_link_gsiQtCore5Compat = _force_link_gsiQtCore5Compat_f ();

#endif
