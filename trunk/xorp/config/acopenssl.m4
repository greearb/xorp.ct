dnl
dnl $XORP$
dnl

dnl
dnl Tests for OpenSSL (Strictly OpenSSL MD5 implementation)
dnl

AC_LANG_PUSH(C)

AC_ARG_WITH([openssl],
	    AC_HELP_STRING([--with-openssl],
			   [specify path to OpenSSL installation root]),
	    [xr_openssl_prefix="$withval"])


fail_openssl()
{
    echo "Could not find part of OpenSSL or one it's components in $1"
    echo "Use --with-openssl=DIR to specify OpenSSL installation root."
    exit 1
}

if test "${xr_openssl_prefix}" = "" ; then
    # User has not specified an OpenSSL prefix, may be in cache, otherwise
    # take a guess.
    AC_CACHE_CHECK([OpenSSL installation prefix],
		   [xr_cv_openssl_prefix],
		   [
		    # Prefer base install over local install, assume base
		    # if not found.
		    xorp_openssl_found="no"
		    xorp_openssl_dirs="/usr /usr/local /usr/local/ssl /mingw /gnuwin32 /opt /usr/sfw"
		    for xr_cv_openssl_prefix in ${xorp_openssl_dirs} ; do
			if test -d ${xr_cv_openssl_prefix}/include/openssl ; then
			    xorp_openssl_found="yes"
			    break
			fi
		    done
		    if test "${xorp_openssl_found}" != "yes" ; then
			fail_openssl "${xorp_openssl_dirs}"
		    fi
		   ]
    )

elif test "${xr_openssl_prefix}" != "${xr_cv_openssl_prefix}" ; then
    # User has specified a new OpenSSL prefix, flush cache state of
    # header files and libraries that we care about to force a
    # re-check.
    unset ac_cv_header_openssl_md5_h
    unset ac_cv_lib_crypto_MD5_Init
    unset xr_cv_openssl_prefix

    # This cache check is doomed to fail because we unset variable,
    # but use it for consistent looking output.
    AC_CACHE_CHECK([OpenSSL installation prefix],
		   [xr_cv_openssl_prefix],
		   [xr_cv_openssl_prefix="${xr_openssl_prefix}";])
fi

if test -d ${xr_cv_openssl_prefix}/include/openssl ; then
    if test "${xr_cv_openssl_prefix}" != "/usr" ; then
	CPPFLAGS="${CPPFLAGS} -I${xr_cv_openssl_prefix}/include"
	LIBS="${LIBS} -L${xr_cv_openssl_prefix} -L${xr_cv_openssl_prefix}/lib"
    fi
    AC_CHECK_HEADER(openssl/md5.h, , fail_openssl "${xr_cv_openssl_prefix}")
    AC_CHECK_LIB(crypto, MD5_Init, , fail_openssl "${xr_cv_openssl_prefix}")
else
    fail_openssl "${xr_cv_openssl_prefix}"
fi


AC_LANG_POP(C)
AC_CACHE_SAVE
