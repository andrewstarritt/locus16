# Allows make to be run from the top level.
#
# SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
# SPDX-License-Identifier: LGPL-3.0-only
#

.PHONY : all install clean uninstall FORCE

# Currently only one sub-directory.
#
SUBDIRS = src

all install clean uninstall: $(SUBDIRS)

# Note the all, clean, uninstall targets are passed from command-line
# via $(MAKECMDGOALS), are handled by each sub-directory's Makefile
# targets.

$(SUBDIRS): FORCE
	$(MAKE) -C $@  $(MAKECMDGOALS) 

# Force targets.
#
FORCE:

# end
