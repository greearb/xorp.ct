dnl
dnl $XORP: xorp/config/acxorp.m4,v 1.4 2003/05/31 15:22:32 pavlin Exp $
dnl

dnl
dnl Autoconf M4 macros for the XORP project.
dnl

AC_DEFUN([XR_TYPE_SIG_T],
[	AC_CACHE_CHECK([for sig_t], xr_cv_type_sig_t,
	[		
	 AC_EGREP_HEADER(sig_t, signal.h, 
	  xr_cv_type_sig_t=yes, xr_cv_type_sig_t=no)
	])
	if test "${xr_cv_type_sig_t}" = "yes"
	then
		AC_DEFINE(HAVE_SIG_T, 1, [Define to 1 if you have sig_t])
	fi
])
