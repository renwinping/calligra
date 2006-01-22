# This file is part of Krita
#
# Copyright (c) 2005-2006 Cyrille Berger <cberger@cberger.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

require "krosskritacore"

doc = Krosskritacore::get("KritaDocument")

image = doc.getImage()
layer = image.getActivePaintLayer()
width = layer.getWidth()
height = layer.getHeight()

filter = Krosskritacore::getFilter("invert")

filter.process(layer, layer)
filter.process(layer, layer, 10, 10, 20, 20 )
	 
