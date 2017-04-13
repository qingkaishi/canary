# generated automatically by aclocal 1.14.1 -*- Autoconf -*-

# Copyright (C) 1996-2013 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

m4_ifndef([AC_CONFIG_MACRO_DIRS], [m4_defun([_AM_CONFIG_MACRO_DIRS], [])m4_defun([AC_CONFIG_MACRO_DIRS], [_AM_CONFIG_MACRO_DIRS($@)])])
#
# Configure a Makefile without clobbering it if it exists and is not out of
# date.  This macro is unique to LLVM.
#
AC_DEFUN([AC_CONFIG_MAKEFILE],
[AC_CONFIG_COMMANDS($1,
  [${llvm_src}/autoconf/mkinstalldirs `dirname $1`
   ${SHELL} ${llvm_src}/autoconf/install-sh -m 0644 -c ${srcdir}/$1 $1])
])

#
# Provide the arguments and other processing needed for an LLVM project
#
AC_DEFUN([LLVM_CONFIG_PROJECT],
  [AC_ARG_WITH([llvmsrc],
    AS_HELP_STRING([--with-llvmsrc],[Location of LLVM Source Code]),
    [llvm_src="$withval"],[llvm_src="]$1["])
  AC_SUBST(LLVM_SRC,$llvm_src)
  AC_ARG_WITH([llvmobj],
    AS_HELP_STRING([--with-llvmobj],[Location of LLVM Object Code]),
    [llvm_obj="$withval"],[llvm_obj="]$2["])
  AC_SUBST(LLVM_OBJ,$llvm_obj)
  AC_CONFIG_COMMANDS([setup],,[llvm_src="${LLVM_SRC}"])
])

#
# Check for the ability to mmap a file.  
#
AC_DEFUN([AC_FUNC_MMAP_FILE],
[AC_CACHE_CHECK(for mmap of files,
ac_cv_func_mmap_file,
[ AC_LANG_PUSH([C])
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
]],[[
  int fd;
  fd = creat ("foo",0777); 
  fd = (int) mmap (0, 1, PROT_READ, MAP_SHARED, fd, 0);
  unlink ("foo"); 
  return (fd != (int) MAP_FAILED);]])],
  [ac_cv_func_mmap_file=yes],[ac_cv_func_mmap_file=no],[ac_cv_func_mmap_file=no])
  AC_LANG_POP([C])
])
if test "$ac_cv_func_mmap_file" = yes; then
   AC_DEFINE([HAVE_MMAP_FILE],[],[Define if mmap() can map files into memory])
   AC_SUBST(MMAP_FILE,[yes])
fi
])

#
# Check for anonymous mmap macros.  This is modified from
# http://www.gnu.org/software/ac-archive/htmldoc/ac_cxx_have_ext_slist.html
#
AC_DEFUN([AC_HEADER_MMAP_ANONYMOUS],
[AC_CACHE_CHECK(for MAP_ANONYMOUS vs. MAP_ANON,
ac_cv_header_mmap_anon,
[ AC_LANG_PUSH([C])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>]],
  [[mmap (0, 1, PROT_READ, MAP_ANONYMOUS, -1, 0); return (0);]])],
  ac_cv_header_mmap_anon=yes, 
  ac_cv_header_mmap_anon=no)
  AC_LANG_POP([C])
])
if test "$ac_cv_header_mmap_anon" = yes; then
   AC_DEFINE([HAVE_MMAP_ANONYMOUS],[1],[Define if mmap() uses MAP_ANONYMOUS to map anonymous pages, or undefine if it uses MAP_ANON])
fi
])

#
# When allocating RWX memory, check whether we need to use /dev/zero
# as the file descriptor or not.
#
AC_DEFUN([AC_NEED_DEV_ZERO_FOR_MMAP],
[AC_CACHE_CHECK([if /dev/zero is needed for mmap],
ac_cv_need_dev_zero_for_mmap,
[if test "$llvm_cv_os_type" = "Interix" ; then
   ac_cv_need_dev_zero_for_mmap=yes
 else
   ac_cv_need_dev_zero_for_mmap=no
 fi
])
if test "$ac_cv_need_dev_zero_for_mmap" = yes; then
  AC_DEFINE([NEED_DEV_ZERO_FOR_MMAP],[1],
   [Define if /dev/zero should be used when mapping RWX memory, or undefine if its not necessary])
fi])

