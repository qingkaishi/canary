import os


def llvm_config(option):
    p = os.popen("llvm-config " + option) 
    x=p.read() 
    return x.strip()


DIRS = ["lib", "tools", "bench"]
EXTRA_DIST = "include"

LLVM_LIBS = []
LLVM_LIB_PATHS = []
ldflags = llvm_config("--ldflags")
ldflags_split = ldflags.split(" ")
for ld in ldflags_split:
    if ld.startswith("-l") is True:
        LLVM_LIBS.append(ld[2:].strip())

    if ld.startswith("-L") is True:
        LLVM_LIB_PATHS.append(ld[2:].strip())


env=Environment(
                CXXFLAGS   = llvm_config("--cxxflags"),
                CFLAGS     = llvm_config("--cflags"),
                CPPFLAGS   = llvm_config("--cppflags"),
                LINKFLAGS  = llvm_config("--ldflags"), 
                CPPPATH    = ["./", "#include/", llvm_config("--includedir")], 
                BIN        = "#bin", 
                LIBPATH    = ['#bin', llvm_config("--libdir")] + LLVM_LIB_PATHS,
                LIBS       = LLVM_LIBS
               )


SCONSCRIPTS = []
for DIR in DIRS:
    SCRIPT = DIR + "/SConscript"
    SCONSCRIPTS.append(SCRIPT)
SConscript(SCONSCRIPTS, exports=['env', 'llvm_config'])

