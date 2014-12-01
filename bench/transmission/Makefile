LEVEL=../..

OBJ=fastresume.bc cli.bc fdlimit.bc peer-msgs.bc completion.bc metainfo.bc inout.bc list.bc blocklist.bc resume.bc ptrarray.bc verify.bc makemeta.bc peer-io.bc platform.bc torrent.bc peer-mgr.bc session.bc trevent.bc bandwidth.bc json.bc webseed.bc crypto.bc wildmat.bc upnp.bc JSON_parser.bc rpcimpl.bc rpc-server.bc ggets.bc port-forwarding.bc tr-getopt.bc ConvertUTF.bc stats.bc natpmp.bc bencode.bc handshake.bc ratecontrol.bc tracker.bc torrent-ctor.bc publish.bc net.bc web.bc utils.bc clients.bc


FLAG=-DPACKAGE_NAME=\"transmission\" -DPACKAGE_TARNAME=\"transmission\" -DPACKAGE_VERSION=\"1.42\" -DPACKAGE_STRING=\"transmission\ 1.42\" -DPACKAGE_BUGREPORT=\"http://trac.transmissionbt.com/newticket\" -DPACKAGE=\"transmission\" -DVERSION=\"1.42\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_FALLOCATE=1 -DHAVE_DAEMON=1 -DHAVE_DIRNAME=1 -DHAVE_BASENAME=1 -DHAVE_DAEMON=1 -DHAVE_STRCASECMP=1 -DHAVE_LOCALTIME_R=1 -DHAVE_POSIX_FALLOCATE=1 -DHAVE_PTHREAD=1 -DHAVE__TMP_DUMMY1_ZLIB_H=1 -DHAVE_ZLIB=1 -D_FILE_OFFSET_BITS=64 -DHAVE_LIBINTL_H=1 -DGETTEXT_PACKAGE=\"transmission\" -DHAVE_LOCALE_H=1 -DHAVE_LC_MESSAGES=1 -DHAVE_BIND_TEXTDOMAIN_CODESET=1 -DHAVE_GETTEXT=1 -DHAVE_DCGETTEXT=1 -DENABLE_NLS=1  -D__TRANSMISSION__ -I. -DPACKAGE_DATA_DIR=\""/usr/local/share"\" -D_GNU_SOURCE

all: transmission.bc libeventdir libnatpmpdir miniupnpdir

libeventdir:
	@$(MAKE) -C ./libevent

libnatpmpdir:
	@$(MAKE) -C ./libnatpmp

miniupnpdir:
	@$(MAKE) -C ./miniupnp

transmission.bc: $(OBJ) libeventdir libnatpmpdir miniupnpdir
	@llvm-link $(OBJ) ./miniupnp/libminiupnp.bc ./libevent/libevent.bc ./libnatpmp/libnatpmp.bc -o transmission.bc

fastresume.bc: fastresume.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./fastresume.c

cli.bc: cli.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./cli.c

fdlimit.bc: fdlimit.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./fdlimit.c

peer-msgs.bc: peer-msgs.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./peer-msgs.c

completion.bc: completion.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./completion.c

metainfo.bc: metainfo.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./metainfo.c

inout.bc: inout.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./inout.c

list.bc: list.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./list.c

blocklist.bc: blocklist.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./blocklist.c

resume.bc: resume.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./resume.c

ptrarray.bc: ptrarray.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./ptrarray.c

verify.bc: verify.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./verify.c

makemeta.bc: makemeta.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./makemeta.c

peer-io.bc:peer-io.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./peer-io.c

platform.bc:platform.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./platform.c

torrent.bc:torrent.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./torrent.c

peer-mgr.bc:peer-mgr.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./peer-mgr.c

session.bc:session.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./session.c

trevent.bc:trevent.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./trevent.c

bandwidth.bc:bandwidth.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./bandwidth.c

json.bc:json.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./json.c

webseed.bc:webseed.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./webseed.c

crypto.bc:crypto.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./crypto.c

wildmat.bc:wildmat.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./wildmat.c

upnp.bc:upnp.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./upnp.c

JSON_parser.bc:JSON_parser.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./JSON_parser.c

rpcimpl.bc: rpcimpl.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./rpcimpl.c

rpc-server.bc:rpc-server.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./rpc-server.c

ggets.bc:ggets.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./ggets.c

port-forwarding.bc:port-forwarding.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./port-forwarding.c

tr-getopt.bc:tr-getopt.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./tr-getopt.c

ConvertUTF.bc:ConvertUTF.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./ConvertUTF.c

stats.bc:stats.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./stats.c

natpmp.bc:natpmp.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./natpmp.c

bencode.bc:bencode.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./bencode.c

handshake.bc:handshake.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./handshake.c

ratecontrol.bc:ratecontrol.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./ratecontrol.c

tracker.bc: tracker.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./tracker.c

torrent-ctor.bc: torrent-ctor.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./torrent-ctor.c

publish.bc: publish.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./publish.c

net.bc: net.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./net.c

web.bc: web.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./web.c

utils.bc:utils.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./utils.c

clients.bc: clients.c
	@clang -c -emit-llvm -fno-vectorize -fno-slp-vectorize $(FLAG) ./clients.c


clean:
	@$(RM) *.bc *.o *.ll *.exe
	@$(MAKE) clean -C libevent
	@$(MAKE) clean -C libnatpmp
	@$(MAKE) clean -C miniupnp

install:
	@echo -n ""

