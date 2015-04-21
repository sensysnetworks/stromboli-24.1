##############################################################################
##
##      Copyright (©) 1999 Zentropic Computing, All rights reserved
##
## Authors:             Steve Papacharalambous (stevep@zentropix.com)
## Original date:       Thu 15 Jul 1999
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation; either
## version 2 of the License, or (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
##
## pthreads interface for Real Time Linux.
##
###############################################################################
K_CFLAGS        = -D__KERNEL__ -DMODULE
CFLAGS          = $(INCLUDES) $(K_CFLAGS) -O2 -g -MD -Wall -Wno-unused -fno-strength-reduce -fomit-frame-pointer
RT_ARCH =$(shell test -f $(LINUX_HOME)/.config; echo $$?)

ifeq ($(RT_ARCH), 0)
	include $(LINUX_HOME)/.config
endif

ifdef CONFIG_SMP
	CFLAGS += -D__SMP__
endif

$(MOD)/%.o : $(OBJ)/%.o
	ld -r -static -o $@ $^
$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@perl -i -pe 's-^(.*:)-$(OUTDIR)/$$1-g' $(<F:.c=.d)
$(MAKE_SUB_DIRS):
	mkdir $@

-include ./*.d
