# Copyright (C) 2005 Richard Altherr <kc8apf@kc8apf.net>
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either 
# version 2 of the License, or (at your option) any later 
# version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

bin_PROGRAMS = pp
pp_SOURCES = debug.c devConfig.c ihx32.c programmer.c pp.c serial.c

sysconf_DATA = pp.conf
man_MANS = pp.1 pp.conf.5

EXTRA_DIST = debug.h devConfig.h ihx32.h programmer.h serial.h $(sysconf_DATA) $(man_MANS)
