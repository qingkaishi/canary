import os
import sys
import string

#--------------------- configure -------------------

LLVM_VERSION_MAJOR = 3
LLVM_VERSION_MINOR = 6

def check_file(file, msg):
    print "Checking for " + file + "."
    if os.path.exists(file) is not True:
        print msg + "\n"
        sys.exit(-1)

def check_file_with_optional_dirs(file_name, dirs, msg):
    print "Checking for " + file_name + "."
    for dir in dirs:
        if os.path.exists(dir + "/" + file_name):
            return

    print msg
    sys.exit(-1)

def check_tool(tool, msg):
    print "Checking for " + tool + "."
    p = os.popen("which " + tool + " 2>/dev/null | head -1") 
    x=p.read() 
    tool_file = x.strip()
    check_file(tool_file, msg)

def check_rough_version(cmd, tool, err_msg, version_major, version_minor):
    print "Checking the version of " + tool + "." 
    p = os.popen(cmd + " 2>/dev/null | head -1") 
    x=p.read() 
    version_string = x.strip()
    ver_str_split = version_string.split(".")
    vmajor = string.atoi(ver_str_split[0].strip())
    vminor = string.atoi(ver_str_split[1].strip())

    if vmajor < version_major:
        print err_msg
        sys.exit(-1)
    elif vmajor == version_major and vminor < version_minor:
        print err_msg
        sys.exit(-1)

def check_precise_version(cmd, tool, err_msg, version_major, version_minor):
    print "Checking the version of " + tool + "." 
    p = os.popen(cmd + " 2>/dev/null | head -1") 
    x=p.read() 
    version_string = x.strip()
    ver_str_split = version_string.split(".")
    vmajor = string.atoi(ver_str_split[0].strip())
    vminor = string.atoi(ver_str_split[1].strip())

    if vmajor != version_major or vminor != version_minor:
        print err_msg
        sys.exit(-1)
    

check_tool("llvm-config", "Error: llvm-config does not exist! Termination.")
check_tool("zip",         "Error: zip does not exist! Termination.")
check_tool("g++",         "Error: g++ does not exist! Termination.")
check_tool("gcc",         "Error: gcc does not exist! Termination.")
check_tool("clang++",     "Error: clang++ does not exist! Termination.")
check_tool("clang",       "Error: clang does not exist! Termination.")

check_precise_version("llvm-config --version", "llvm",
                      "Error: llvm's version should be " + str(LLVM_VERSION_MAJOR) + "." + str(LLVM_VERSION_MINOR) + "! Termination.",
                      LLVM_VERSION_MAJOR, LLVM_VERSION_MINOR)

check_rough_version("gcc -dumpversion", "gcc",
                    "Error: gcc's version should be >= 4.8! Termination.",
                    4, 8)

#check_file_with_optional_dirs("boost/foreach.hpp", 
#                              ["/usr/include", "/usr/local/include"], 
#                              "Error: boost library is needed for support. Termination.")

#--------------------- build -----------------------

def llvm_config(option):
    p = os.popen("llvm-config " + option) 
    x=p.read() 
    return x.strip()


DIRS = ["lib", "tools"]
EXTRA_DIST = "include"


LLVM_LIB_PATHS = []
ldflags = llvm_config("--ldflags")
ldflags_split = ldflags.split(" ")
for ld in ldflags_split:
    if ld.startswith("-L") is True:
        LLVM_LIB_PATHS.append(ld[2:].strip())


syslibs = llvm_config("--system-libs")
syslibs_split = syslibs.split(" ")
LLVM_LIBS = []
for syslib in syslibs_split:
    if syslib.startswith("-l") is True:
        LLVM_LIBS.append(syslib[2:].strip())

env=Environment(
                ENV        = os.environ,
                CXX        = "g++",
                CC         = "gcc",
                CXXFLAGS   = llvm_config("--cxxflags"),
                CFLAGS     = llvm_config("--cflags"),
                CPPFLAGS   = llvm_config("--cppflags"),
                LINKFLAGS  = llvm_config("--ldflags"), 
                CPPPATH    = ["./", "#include/", llvm_config("--includedir")], 
                BIN        = "#bin", 
                LIBPATH    = ['#bin', llvm_config("--libdir")] + LLVM_LIB_PATHS,
                LIBS       = LLVM_LIBS
               )

BuildDirs=[]
libonly = ARGUMENTS.get("libonly", 0)
if int(libonly):
	BuildDirs.append("lib")
else:
	for DIR in DIRS:
		BuildDirs.append(DIR)

SCONSCRIPTS = []
for DIR in BuildDirs:
    SCRIPT = DIR + "/SConscript"
    SCONSCRIPTS.append(SCRIPT)
SConscript(SCONSCRIPTS, exports=['env', 'llvm_config'])

