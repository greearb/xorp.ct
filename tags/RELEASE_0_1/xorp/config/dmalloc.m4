## ----------------------------------- ##
## Check if --with-dmalloc was given.  ##
## From Franc,ois Pinard               ##
## ----------------------------------- ##
## Modified by Pavlin Radoslavov:      ##
##   - can specify dmalloc directory   ##
##   - `-ldmalloc' before `$LIBS'      ##
##   - correct dmalloc URL             ##


# serial 1

AC_DEFUN(XR_WITH_DMALLOC_DIR,
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
[  --with-dmalloc[=DIR]    use dmalloc (http://www.dmalloc.com/) from DIR],
[if test "$withval" = no; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC,1,
            [Define if using the dmalloc debugging malloc package])
  if test "$withval" = yes; then
    LIBS="-ldmalloc $LIBS"
  else
    LIBS="-L$withval -L$withval/lib -ldmalloc $LIBS"
    CFLAGS="$CFLAGS -I$withval -I$withval/include"
  fi
  LDFLAGS="$LDFLAGS -g"
fi], [AC_MSG_RESULT(no)])
])
