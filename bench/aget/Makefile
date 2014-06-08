##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..
LLVMSRCS = main.c Aget.c Misc.c Head.c Signal.c Download.c Ftp.c Resume.c loadrc.c
LDFLAGS = -pthread -D_GNU_SOURCE

all: client aget.bc

client:
	@$(MAKE) -C reproduce-pkg

aget.bc: main.bc Aget.bc Misc.bc Head.bc Signal.bc Download.bc Ftp.bc Resume.bc loadrc.bc
	@llvm-link main.bc Aget.bc Misc.bc Head.bc Signal.bc Download.bc Ftp.bc Resume.bc loadrc.bc -o aget.bc

main.bc: main.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize main.c -o main.bc $(LDFLAGS)

Aget.bc: Aget.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Aget.c -o Aget.bc $(LDFLAGS)

Misc.bc: Misc.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Misc.c -o Misc.bc $(LDFLAGS)

Head.bc: Head.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Head.c -o Head.bc $(LDFLAGS)

Signal.bc: Signal.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Signal.c -o Signal.bc $(LDFLAGS)

Download.bc: Download.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Download.c -o Download.bc $(LDFLAGS)

Ftp.bc: Ftp.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Ftp.c -o Ftp.bc $(LDFLAGS)

Resume.bc: Resume.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize Resume.c -o Resume.bc $(LDFLAGS)

loadrc.bc: loadrc.c
	@clang -c -g -O2 -emit-llvm -fno-vectorize loadrc.c -o loadrc.bc $(LDFLAGS)

clean:
	@$(RM) *.bc *.o *.ll *.exe 
	@$(MAKE) clean -C reproduce-pkg

install:
	@echo -n ""
