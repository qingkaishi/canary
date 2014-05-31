##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..
LLVMSRCS = main.c Aget.c Misc.c Head.c Signal.c Download.c Ftp.c Resume.c loadrc.c
LDFLAGS = -pthread -D_GNU_SOURCE

all: $(LLVMSRCS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize main.c -o main.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Aget.c -o Aget.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Misc.c -o Misc.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Head.c -o Head.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Signal.c -o Signal.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Download.c -o Download.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Ftp.c -o Ftp.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize Resume.c -o Resume.bc $(LDFLAGS)
	@clang -c -g -O2 -emit-llvm -fno-vectorize loadrc.c -o loadrc.bc $(LDFLAGS)
	@llvm-link main.bc Aget.bc Misc.bc Head.bc Signal.bc Download.bc Ftp.bc Resume.bc loadrc.bc -o aget.bc
	@$(MAKE) -C reproduce-pkg

clean:
	@$(RM) *.bc *.o *.ll 

install:
	@echo -n ""
