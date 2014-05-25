LEVEL=../..

all:
	@$(MAKE) -C ./libevent
	@$(MAKE) -C ./libnatpmp
	@$(MAKE) -C ./miniupnp
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize *.c -DPACKAGE_NAME=\"transmission\" -DPACKAGE_TARNAME=\"transmission\" -DPACKAGE_VERSION=\"1.42\" -DPACKAGE_STRING=\"transmission\ 1.42\" -DPACKAGE_BUGREPORT=\"http://trac.transmissionbt.com/newticket\" -DPACKAGE=\"transmission\" -DVERSION=\"1.42\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_FALLOCATE=1 -DHAVE_DAEMON=1 -DHAVE_DIRNAME=1 -DHAVE_BASENAME=1 -DHAVE_DAEMON=1 -DHAVE_STRCASECMP=1 -DHAVE_LOCALTIME_R=1 -DHAVE_POSIX_FALLOCATE=1 -DHAVE_PTHREAD=1 -DHAVE__TMP_DUMMY1_ZLIB_H=1 -DHAVE_ZLIB=1 -D_FILE_OFFSET_BITS=64 -DHAVE_LIBINTL_H=1 -DGETTEXT_PACKAGE=\"transmission\" -DHAVE_LOCALE_H=1 -DHAVE_LC_MESSAGES=1 -DHAVE_BIND_TEXTDOMAIN_CODESET=1 -DHAVE_GETTEXT=1 -DHAVE_DCGETTEXT=1 -DENABLE_NLS=1  -D__TRANSMISSION__ -I. -DPACKAGE_DATA_DIR=\""/usr/local/share"\" -D_GNU_SOURCE
	@$(RM) transmission.bc
	@llvm-link *.bc ./miniupnp/libminiupnp.bc ./libevent/libevent.bc ./libnatpmp/libnatpmp.bc -o transmission.bc #

clean:
	@$(RM) *.bc *.o *.ll *.exe

install:
	@echo -n ""

