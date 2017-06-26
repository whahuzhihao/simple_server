dnl $Id$
dnl config.m4 for extension simple_server

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(simple_server, for simple_server support,
dnl Make sure that the comment is aligned:
dnl [  --with-simple_server             Include simple_server support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(simple_server, whether to enable simple_server support,
Make sure that the comment is aligned:
[  --enable-simple_server           Enable simple_server support])

if test "$PHP_SIMPLE_SERVER" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-simple_server -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/simple_server.h"  # you most likely want to change this
  dnl if test -r $PHP_SIMPLE_SERVER/$SEARCH_FOR; then # path given as parameter
  dnl   SIMPLE_SERVER_DIR=$PHP_SIMPLE_SERVER
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for simple_server files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       SIMPLE_SERVER_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$SIMPLE_SERVER_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the simple_server distribution])
  dnl fi

  dnl # --with-simple_server -> add include path
  dnl PHP_ADD_INCLUDE($SIMPLE_SERVER_DIR/include)

  dnl # --with-simple_server -> check for lib and symbol presence
  dnl LIBNAME=simple_server # you may want to change this
  dnl LIBSYMBOL=simple_server # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SIMPLE_SERVER_DIR/$PHP_LIBDIR, SIMPLE_SERVER_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_SIMPLE_SERVERLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong simple_server lib version or lib not found])
  dnl ],[
  dnl   -L$SIMPLE_SERVER_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl

  dnl CXX_FLAGS="-std=c++0x"
  PHP_REQUIRE_CXX()
  PHP_ADD_INCLUDE(src/include .)
  PHP_ADD_LIBRARY(stdc++, 1, SIMPLE_SERVER_SHARED_LIBADD)
  PHP_ADD_LIBRARY(event, 1, SIMPLE_SERVER_SHARED_LIBADD)
  PHP_ADD_LIBRARY(pthread, 1, SIMPLE_SERVER_SHARED_LIBADD)
  PHP_SUBST(SIMPLE_SERVER_SHARED_LIBADD)

  source_file="src/server.cpp \
    src/core/tcp_event_server.cpp \
    simple_server.cpp"

  PHP_NEW_EXTENSION(simple_server, $source_file , $ext_shared)
fi

if test -z "$PHP_DEBUG" ; then
    AC_ARG_ENABLE(debug, [--enable-debug compile with debugging system], [PHP_DEBUG=$enableval],[PHP_DEBUG=no] )
fi