dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([[[Canary]]],[[[1.0]]],[report@sbrella.com])

dnl Tell autoconf that the auxilliary files are actually located in
dnl the LLVM autoconf directory, not here.
AC_CONFIG_AUX_DIR(autoconf)

#AC_CONFIG_MACRO_DIR($srcdir/autoconf)

dnl Check if llvm has installed system wide
AC_MSG_CHECKING([llvm-config])
llvm_config_path="`which llvm-config 2> /dev/null | head -1`"
if ! test -f "$llvm_config_path" ; then
   llvm_config_path="no"
   #AC_MSG_ERROR([It seems llvm is not correctly installed. llvm-config cannot be found.])
fi
AC_MSG_RESULT([$llvm_config_path])

dnl Identify where LLVM source tree is
LLVM_SRC_ROOT="unknown" #"`(cd $srcdir/../..; pwd)`"
LLVM_OBJ_ROOT="unknown"

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT($LLVM_SRC_ROOT,$LLVM_OBJ_ROOT)

# LLVM_SRC and LLVM_OBJ are two variables introduced by the macro LLVM_CONFIG_PROJECT
if [[ "$LLVM_SRC" == "unknown" ]] ; then
   if [[ "$llvm_config_path" == "no" ]] ; then
      AC_MSG_ERROR([Please specify the LLVM source tree path by --with-llvmsrc])
   else
      LLVM_SRC="`llvm-config --src-root`"
   fi
fi

if [[ "$LLVM_OBJ" == "unknown" ]] ; then
   if [[ "$llvm_config_path" == "no" ]] ; then
      AC_MSG_ERROR([Please specify the LLVM build path by --with-llvmobj])
   else
      LLVM_SRC="`llvm-config --obj-root`"
   fi
fi

# Obtain the absolute path	
LLVM_SRC="`(cd $LLVM_SRC; pwd)`"
LLVM_OBJ="`(cd $LLVM_OBJ; pwd)`"

dnl Specify extra linker build options
AC_ARG_WITH(extra-ld-options,
  AS_HELP_STRING([--with-extra-ld-options],
                 [Specify additional options to link LLVM with]),,
                 withval=default)
case "$withval" in
   default) EXTRA_LD_OPTIONS= ;;
   *) EXTRA_LD_OPTIONS=$withval ;;
esac
AC_SUBST(EXTRA_LD_OPTIONS,$EXTRA_LD_OPTIONS)

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.common.in"])

dnl Configure Makefile common files
AC_CONFIG_FILES(Makefile.common)

dnl Configure project makefiles
dnl List every Makefile that exists within your source tree
AC_CONFIG_MAKEFILE(Makefile)

dnl **************************************************************************
dnl * Check for programs.
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for libraries.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for header files.
dnl **************************************************************************
AC_HEADER_MMAP_ANONYMOUS
AC_HEADER_STAT
AC_CHECK_HEADERS([fcntl.h sys/mman.h])

dnl **************************************************************************
dnl * Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for library functions.
dnl **************************************************************************
AC_FUNC_MMAP
AC_FUNC_MMAP_FILE
AC_NEED_DEV_ZERO_FOR_MMAP

dnl This must be last
AC_OUTPUT
