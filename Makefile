# Allows make to be run from the top level.
#

.PHONY : all clean  uninstall FORCE

# Currently only one sub-directory.
#
SUBDIRS = src

all clean uninstall: $(SUBDIRS)

# Note the all, clean, uninstall targets are passed from command-line
# via $(MAKECMDGOALS), are handled by each sub-directory's Makefile
# targets.

$(SUBDIRS): FORCE
	$(MAKE) -C $@  $(MAKECMDGOALS) 

# Force targets.
#
FORCE:

# end
