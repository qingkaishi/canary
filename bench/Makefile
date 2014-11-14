
#
# List all of the subdirectories that we will compile.
#
DIRS=simplerace bbuf swarm pbzip2 aget pfscan racey canneal memcached transmission

all:
	$(foreach VAR,$(DIRS),$(MAKE) -C $(VAR);) 
        
clean:
	$(foreach VAR,$(DIRS),$(MAKE) clean -C $(VAR);)

