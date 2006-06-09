dnl
dnl $XORP: xorp/config/compiler_flags.m4,v 1.6 2006/01/17 23:03:40 pavlin Exp $
dnl

dnl
dnl Check for C compiler command-line options
dnl

dnl
dnl XR_CHECK_CFLAG(COMPILER-FLAG, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([XR_CHECK_CFLAG],
[dnl Do the transliteration at runtime so arg 1 can be a shell variable.
  ac_safe=`echo "$1" | sed 'y%./+- ,%__p___%'`
  AC_LANG_PUSH(C)
  _save_flags="$CFLAGS"
  CFLAGS="$CFLAGS $1"
  AC_MSG_CHECKING([whether C compiler supports flag "$1"])
  AC_CACHE_VAL(ac_cv_prog_c_compiler_$ac_safe,
    [AC_COMPILE_IFELSE([
int
main(int argc, char **argv)
{
	if (sizeof(argc) + sizeof(argv) == 0)
		return (1);
	return (0);
}
],
      [eval "ac_cv_prog_c_compiler_$ac_safe=yes"
      dnl AC_MSG_RESULT(yes)
      ],
      [eval "ac_cv_prog_c_compiler_$ac_safe=no"
      dnl AC_MSG_RESULT(no)
    ])
  ])
  CFLAGS="$_save_flags"
  AC_LANG_POP(C)
  if eval "test \"`echo '$ac_cv_prog_c_compiler_'$ac_safe`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$2], , :, [$2])
  else
    AC_MSG_RESULT(no)
    ifelse([$3], , , [$3])
  fi
])

dnl
dnl XR_CHECK_CFLAGS(COMPILER-FLAG... [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([XR_CHECK_CFLAGS],
[for ac_flag in $1
  do
    XR_CHECK_CFLAG($ac_flag, $2, $3)
  done
])

dnl
dnl XR_TRY_ADD_CFLAGS(COMPILER-FLAG...)
dnl
dnl Conditionally add each of COMPILER-FLAG (if supported) to CFLAGS
dnl
AC_DEFUN([XR_TRY_ADD_CFLAGS],
[for ac_flag in $1
  do
    XR_CHECK_CFLAG($ac_flag, [CFLAGS="$CFLAGS $ac_flag"])
  done
])



dnl
dnl Check for C++ complier command-line options
dnl

dnl
dnl XR_CHECK_CXXFLAG(COMPILER-FLAG, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([XR_CHECK_CXXFLAG],
[dnl Do the transliteration at runtime so arg 1 can be a shell variable.
  ac_safe=`echo "$1" | sed 'y%./+- %__p__%'`
  AC_LANG_PUSH(C++)
  _save_flags="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $1"
  AC_MSG_CHECKING([whether C++ compiler supports flag "$1"])
  AC_CACHE_VAL(ac_cv_prog_cxx_compiler_$ac_safe,
    [AC_COMPILE_IFELSE([
int
main(int argc, char **argv)
{
	if (sizeof(argc) + sizeof(argv) == 0)
		return (1);
	return (0);
}
],
      [eval "ac_cv_prog_cxx_compiler_$ac_safe=yes"
      dnl AC_MSG_RESULT(yes)
      ],
      [eval "ac_cv_prog_cxx_compiler_$ac_safe=no"
      dnl AC_MSG_RESULT(no)
    ])
  ])
  CXXFLAGS="$_save_flags"
  AC_LANG_POP(C++)
  if eval "test \"`echo '$ac_cv_prog_cxx_compiler_'$ac_safe`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$2], , :, [$2])
  else
    AC_MSG_RESULT(no)
    ifelse([$3], , , [$3])
  fi
])

dnl
dnl XR_CHECK_CXXFLAGS(COMPILER-FLAG... [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([XR_CHECK_CXXFLAGS],
[for ac_flag in $1
  do
    XR_CHECK_CXXFLAG($ac_flag, $2, $3)
  done
])

dnl
dnl XR_TRY_ADD_CXXFLAGS(COMPILER-FLAG...)
dnl
dnl Conditionally add each of COMPILER-FLAG (if supported) to CXXFLAGS
dnl
AC_DEFUN([XR_TRY_ADD_CXXFLAGS],
[for ac_flag in $1
  do
    XR_CHECK_CXXFLAG($ac_flag, [CXXFLAGS="$CXXFLAGS $ac_flag"])
  done
])


AC_CACHE_SAVE
