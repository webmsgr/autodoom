// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Ioan Chera
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
// Additional terms and conditions compatible with the GPLv3 apply. See the
// file COPYING-EE for details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      Line effects on the map, by their type numbers. Specified separately
//      from Eternity's internals because the latter is going too deep.
//
//-----------------------------------------------------------------------------

#ifndef __EternityEngine__b_lineeffect__
#define __EternityEngine__b_lineeffect__

struct line_t;
struct sector_t;

namespace LevelStateStack
{
   void     InitLevel();
   bool     Push(const line_t& line, int action, int tag);
   void     Pop();
   void     Clear();
   fixed_t  Floor(const sector_t& sector);
   fixed_t  Ceiling(const sector_t& sector);
}

#endif /* defined(__EternityEngine__b_lineeffect__) */