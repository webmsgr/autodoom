// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  Functions to manipulate linear blocks of graphics data.
//
//----------------------------------------------------------------------------

#ifndef V_BLOCK_H__
#define V_BLOCK_H__

// Scaled color block drawers.

void V_ColorBlockScaled(VBuffer *buffer, byte color, int x, int y, int w, int h);

void V_ColorBlockTLScaled(VBuffer *dest, byte color, int x, int y, int w, int h, 
                          int tl);

// haleyjd 02/02/05: color block drawing functions

void V_ColorBlock(VBuffer *buffer, byte color, int x, int y, int w, int h);

void V_ColorBlockTL(VBuffer *buffer, byte color, int x, int y, int w, 
                    int h, int tl);

// sets block function pointers for a VBuffer object
void V_SetBlockFuncs(VBuffer *, int);

#endif

// EOF


