// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2013 James Haley et al.
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Movement/collision utility functions,
//      as used by function in p_map.c.
//      BLOCKMAP Iterator functions,
//      and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"

#include "doomstat.h"
#include "m_bbox.h"
#include "p_map.h"
#include "p_map3d.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "polyobj.h"
#include "r_data.h"
#include "r_main.h"
#include "r_portal.h"
#include "r_state.h"


//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//
fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
   dx = D_abs(dx);
   dy = D_abs(dy);
   if(dx < dy)
      return dx+dy-(dx>>1);
   return dx+dy-(dy>>1);
}

//
// P_PointOnLineSide
// Returns 0 or 1
//
// killough 5/3/98: reformatted, cleaned up
//
int P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line)
{
   return
      !line->dx ? x <= line->v1->x ? line->dy > 0 : line->dy < 0 :
      !line->dy ? y <= line->v1->y ? line->dx < 0 : line->dx > 0 :
      FixedMul(y-line->v1->y, line->dx>>FRACBITS) >=
      FixedMul(line->dy>>FRACBITS, x-line->v1->x);
}

//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
// killough 5/3/98: reformatted, cleaned up
//
int P_BoxOnLineSide(fixed_t *tmbox, line_t *ld)
{
   int p;

   switch (ld->slopetype)
   {
   default: // shut up compiler warnings -- killough
   case ST_HORIZONTAL:
      return
      (tmbox[BOXBOTTOM] > ld->v1->y) == (p = tmbox[BOXTOP] > ld->v1->y) ?
        p ^ (ld->dx < 0) : -1;
   case ST_VERTICAL:
      return
        (tmbox[BOXLEFT] < ld->v1->x) == (p = tmbox[BOXRIGHT] < ld->v1->x) ?
        p ^ (ld->dy < 0) : -1;
   case ST_POSITIVE:
      return
        P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld) ==
        (p = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXTOP], ld)) ? p : -1;
   case ST_NEGATIVE:
      return
        (P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld)) ==
        (p = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld)) ? p : -1;
    }
}

//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
// killough 5/3/98: reformatted, cleaned up
//
int P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line)
{
   return
      !line->dx ? x <= line->x ? line->dy > 0 : line->dy < 0 :
      !line->dy ? y <= line->y ? line->dx < 0 : line->dx > 0 :
      (line->dy^line->dx^(x -= line->x)^(y -= line->y)) < 0 ? (line->dy^x) < 0 :
      FixedMul(y>>8, line->dx>>8) >= FixedMul(line->dy>>8, x>>8);
}

//
// P_MakeDivline
//
void P_MakeDivline(line_t *li, divline_t *dl)
{
   dl->x  = li->v1->x;
   dl->y  = li->v1->y;
   dl->dx = li->dx;
   dl->dy = li->dy;
}

//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
// killough 5/3/98: reformatted, cleaned up
//
fixed_t P_InterceptVector(divline_t *v2, divline_t *v1)
{
   fixed_t den = FixedMul(v1->dy>>8, v2->dx) - FixedMul(v1->dx>>8, v2->dy);
   return den ? FixedDiv((FixedMul((v1->x-v2->x)>>8, v1->dy) +
                          FixedMul((v2->y-v1->y)>>8, v1->dx)), den) : 0;
}

//
// P_LineOpening
//
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//
void P_LineOpening(line_t *linedef, Mobj *mo)
{
   fixed_t frontceilz, frontfloorz, backceilz, backfloorz;
   // SoM: used for 3dmidtex
   fixed_t frontcz, frontfz, backcz, backfz, otop, obot;

   if(linedef->sidenum[1] == -1)      // single sided line
   {
      clip.openrange = 0;
      return;
   }
   
   clip.openfrontsector = linedef->frontsector;
   clip.openbacksector  = linedef->backsector;

   // SoM: ok, new plan. The only way a 2s line should give a lowered floor or hightened ceiling
   // z is if both sides of that line have the same portal.
   {
#ifdef R_LINKEDPORTALS
      if(mo && demo_version >= 333 && 
         clip.openfrontsector->c_pflags & PS_PASSABLE &&
         clip.openbacksector->c_pflags & PS_PASSABLE && 
         clip.openfrontsector->c_portal == clip.openbacksector->c_portal)
      {
         frontceilz = backceilz = clip.openfrontsector->ceilingheight + (1024 * FRACUNIT);
      }
      else
#endif
      {
         frontceilz = clip.openfrontsector->ceilingheight;
         backceilz  = clip.openbacksector->ceilingheight;
      }
      
      frontcz = clip.openfrontsector->ceilingheight;
      backcz  = clip.openbacksector->ceilingheight;
   }


   {
#ifdef R_LINKEDPORTALS
      if(mo && demo_version >= 333 && 
         clip.openfrontsector->f_pflags & PS_PASSABLE &&
         clip.openbacksector->f_pflags & PS_PASSABLE && 
         clip.openfrontsector->f_portal == clip.openbacksector->f_portal)
      {
         frontfloorz = backfloorz = clip.openfrontsector->floorheight - (1024 * FRACUNIT); //mo->height;
      }
      else 
#endif
      {
         frontfloorz = clip.openfrontsector->floorheight;
         backfloorz  = clip.openbacksector->floorheight;
      }

      frontfz = clip.openfrontsector->floorheight;
      backfz = clip.openbacksector->floorheight;
   }
   
   if(frontceilz < backceilz)
      clip.opentop = frontceilz;
   else
      clip.opentop = backceilz;

   
   if(frontfloorz > backfloorz)
   {
      clip.openbottom = frontfloorz;
      clip.lowfloor = backfloorz;
      // haleyjd
      clip.floorpic = clip.openfrontsector->floorpic;
   }
   else
   {
      clip.openbottom = backfloorz;
      clip.lowfloor = frontfloorz;
      // haleyjd
      clip.floorpic = clip.openbacksector->floorpic;
   }

   if(frontcz < backcz)
      otop = frontcz;
   else
      otop = backcz;

   if(frontfz > backfz)
      obot = frontfz;
   else
      obot = backfz;

   clip.opensecfloor = clip.openbottom;
   clip.opensecceil  = clip.opentop;

   // SoM 9/02/02: Um... I know I told Quasar` I would do this after 
   // I got SDL_Mixer support and all, but I WANT THIS NOW hehe
   if(demo_version >= 331 && mo && (linedef->flags & ML_3DMIDTEX) && 
      sides[linedef->sidenum[0]].midtexture)
   {
      fixed_t textop, texbot, texmid;
      side_t *side = &sides[linedef->sidenum[0]];
      
      if(linedef->flags & ML_DONTPEGBOTTOM)
      {
         texbot = side->rowoffset + obot;
         textop = texbot + textures[side->midtexture]->heightfrac;
      }
      else
      {
         textop = otop + side->rowoffset;
         texbot = textop - textures[side->midtexture]->heightfrac;
      }
      texmid = (textop + texbot)/2;

      // SoM 9/7/02: use monster blocking line to provide better
      // clipping
      if((linedef->flags & ML_BLOCKMONSTERS) && 
         !(mo->flags & (MF_FLOAT | MF_DROPOFF)) &&
         D_abs(mo->z - textop) <= 24*FRACUNIT)
      {
         clip.opentop = clip.openbottom;
         clip.openrange = 0;
         return;
      }
      
      if(mo->z + (P_ThingInfoHeight(mo->info) / 2) < texmid)
      {
         if(texbot < clip.opentop)
            clip.opentop = texbot;
      }
      else
      {
         if(textop > clip.openbottom)
            clip.openbottom = textop;

         // The mobj is above the 3DMidTex, so check to see if it's ON the 3DMidTex
         // SoM 01/12/06: let monsters walk over dropoffs
         if(abs(mo->z - textop) <= 24*FRACUNIT)
            clip.touch3dside = 1;
      }
   }

   clip.openrange = clip.opentop - clip.openbottom;
}

//
// THING POSITION SETTING
//

//
// P_LogThingPosition
//
// haleyjd 04/15/2010: thing position logging for debugging demo problems.
// Pass a NULL mobj to close the log.
//
#ifdef THING_LOGGING
void P_LogThingPosition(Mobj *mo, const char *caller)
{
   static FILE *thinglog;

   if(!thinglog)
      thinglog = fopen("thinglog.txt", "w");

   if(!mo)
   {
      if(thinglog)
         fclose(thinglog);
      thinglog = NULL;
      return;
   }

   if(thinglog)
   {
      fprintf(thinglog,
         "%010d:%s:%p:%20s:%+010d:%+010d:%+010d:%+010d\n",
         gametic, caller, mo, mo->info->name, mo->x, mo->y, mo->z, mo->flags);
   }
}
#else
#define P_LogThingPosition(a, b)
#endif

//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition(Mobj *thing)
{
   P_LogThingPosition(thing, "unset");

   if(!(thing->flags & MF_NOSECTOR))
   {
      // invisible things don't need to be in sector list
      // unlink from subsector
      
      // killough 8/11/98: simpler scheme using pointers-to-pointers for prev
      // pointers, allows head node pointers to be treated like everything else
      Mobj **sprev = thing->sprev;
      Mobj  *snext = thing->snext;
      if((*sprev = snext))  // unlink from sector list
         snext->sprev = sprev;

      // phares 3/14/98
      //
      // Save the sector list pointed to by touching_sectorlist.
      // In P_SetThingPosition, we'll keep any nodes that represent
      // sectors the Thing still touches. We'll add new ones then, and
      // delete any nodes for sectors the Thing has vacated. Then we'll
      // put it back into touching_sectorlist. It's done this way to
      // avoid a lot of deleting/creating for nodes, when most of the
      // time you just get back what you deleted anyway.
      //
      // If this Thing is being removed entirely, then the calling
      // routine will clear out the nodes in sector_list.
      
      thing->old_sectorlist = thing->touching_sectorlist;
      thing->touching_sectorlist = NULL; // to be restored by P_SetThingPosition
   }

   if(!(thing->flags & MF_NOBLOCKMAP))
   {
      // inert things don't need to be in blockmap
      
      // killough 8/11/98: simpler scheme using pointers-to-pointers for prev
      // pointers, allows head node pointers to be treated like everything else
      //
      // Also more robust, since it doesn't depend on current position for
      // unlinking. Old method required computing head node based on position
      // at time of unlinking, assuming it was the same position as during
      // linking.
      
      Mobj *bnext, **bprev = thing->bprev;
      if(bprev && (*bprev = bnext = thing->bnext))  // unlink from block map
         bnext->bprev = bprev;
   }
}

//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
// killough 5/3/98: reformatted, cleaned up
//
void P_SetThingPosition(Mobj *thing)
{
   // link into subsector
   subsector_t *ss = thing->subsector = R_PointInSubsector(thing->x, thing->y);

   P_LogThingPosition(thing, " set ");

#ifdef R_LINKEDPORTALS
   thing->groupid = ss->sector->groupid;
#endif

   if(!(thing->flags & MF_NOSECTOR))
   {
      // invisible things don't go into the sector links
      
      // killough 8/11/98: simpler scheme using pointer-to-pointer prev
      // pointers, allows head nodes to be treated like everything else
      
      Mobj **link = &ss->sector->thinglist;
      Mobj *snext = *link;
      if((thing->snext = snext))
         snext->sprev = &thing->snext;
      thing->sprev = link;
      *link = thing;

      // phares 3/16/98
      //
      // If sector_list isn't NULL, it has a collection of sector
      // nodes that were just removed from this Thing.
      //
      // Collect the sectors the object will live in by looking at
      // the existing sector_list and adding new nodes and deleting
      // obsolete ones.
      //
      // When a node is deleted, its sector links (the links starting
      // at sector_t->touching_thinglist) are broken. When a node is
      // added, new sector links are created.

      thing->touching_sectorlist = P_CreateSecNodeList(thing, thing->x, thing->y);
      thing->old_sectorlist = NULL;
   }

   // link into blockmap
   if(!(thing->flags & MF_NOBLOCKMAP))
   {
      // inert things don't need to be in blockmap
      int blockx = (thing->x - bmaporgx) >> MAPBLOCKSHIFT;
      int blocky = (thing->y - bmaporgy) >> MAPBLOCKSHIFT;
      
      if(blockx >= 0 && blockx < bmapwidth && blocky >= 0 && blocky < bmapheight)
      {
         // killough 8/11/98: simpler scheme using pointer-to-pointer prev
         // pointers, allows head nodes to be treated like everything else

         Mobj **link = &blocklinks[blocky*bmapwidth+blockx];
         Mobj *bnext = *link;
         if((thing->bnext = bnext))
            bnext->bprev = &thing->bnext;
         thing->bprev = link;
         *link = thing;
      }
      else        // thing is off the map
         thing->bnext = NULL, thing->bprev = NULL;
   }
}

// killough 3/15/98:
//
// A fast function for testing intersections between things and linedefs.
//
// haleyjd: note -- this is never called, and is, according to
// SoM, VERY inaccurate. I don't really know what its for or why
// its here, but I'm leaving it be.
//
bool ThingIsOnLine(Mobj *t, line_t *l)
{
   int dx = l->dx >> FRACBITS;                           // Linedef vector
   int dy = l->dy >> FRACBITS;
   int a = (l->v1->x >> FRACBITS) - (t->x >> FRACBITS);  // Thing-->v1 vector
   int b = (l->v1->y >> FRACBITS) - (t->y >> FRACBITS);
   int r = t->radius >> FRACBITS;                        // Thing radius

   // First make sure bounding boxes of linedef and thing intersect.
   // Leads to quick rejection using only shifts and adds/subs/compares.
   
   if(D_abs(a*2+dx)-D_abs(dx) > r*2 || D_abs(b*2+dy)-D_abs(dy) > r*2)
      return 0;

   // Next, make sure that at least one thing crosshair intersects linedef's
   // extension. Requires only 3-4 multiplications, the rest adds/subs/
   // shifts/xors (writing the steps out this way leads to better codegen).

   a *= dy;
   b *= dx;
   a -= b;
   b = dx + dy;
   b *= r;
   if(((a-b)^(a+b)) < 0)
      return 1;
   dy -= dx;
   dy *= r;
   b = a+dy;
   a -= dy;
   return (a^b) < 0;
}

//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//

//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
// killough 5/3/98: reformatted, cleaned up
//
bool P_BlockLinesIterator(int x, int y, bool func(line_t*))
{
   int        offset;
   const int  *list;     // killough 3/1/98: for removal of blockmap limit
   DLListItem<polymaplink_t> *plink; // haleyjd 02/22/06
   
   if(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
      return true;
   offset = y * bmapwidth + x;

   // haleyjd 02/22/06: consider polyobject lines
   plink = polyblocklinks[offset];

   while(plink)
   {
      polyobj_t *po = (*plink)->po;

      if(po->validcount != validcount) // if polyobj hasn't been checked
      {
         int i;
         po->validcount = validcount;
         
         for(i = 0; i < po->numLines; ++i)
         {
            if(po->lines[i]->validcount == validcount) // line has been checked
               continue;
            po->lines[i]->validcount = validcount;
            if(!func(po->lines[i]))
               return false;
         }
      }
      plink = plink->dllNext;
   }

   // original was reading delimiting 0 as linedef 0 -- phares
   offset = *(blockmap + offset);
   list = blockmaplump + offset;

   // killough 1/31/98: for compatibility we need to use the old method.
   // Most demos go out of sync, and maybe other problems happen, if we
   // don't consider linedef 0. For safety this should be qualified.

   // killough 2/22/98: demo_compatibility check
   // skip 0 starting delimiter -- phares
   if(!demo_compatibility)
      list++;     
   for( ; *list != -1; list++)
   {
      line_t *ld;
      
      // haleyjd 04/06/10: to avoid some crashes during demo playback due to
      // invalid blockmap lumps
      if(*list >= numlines)
         continue;

      ld = &lines[*list];
      if(ld->validcount == validcount)
         continue;       // line has already been checked
      ld->validcount = validcount;
      if(!func(ld))
         return false;
   }
   return true;  // everything was checked
}

//
// P_BlockThingsIterator
//
// killough 5/3/98: reformatted, cleaned up
//
bool P_BlockThingsIterator(int x, int y, bool func(Mobj*))
{
   if(!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
   {
      Mobj *mobj = blocklinks[y * bmapwidth + x];

      for(; mobj; mobj = mobj->bnext)
         if(!func(mobj))
            return false;
   }
   return true;
}

//
// P_PointToAngle
//
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up
// haleyjd 01/28/10: restored to Vanilla and made some modifications;
//                   added P_ version for use by gamecode.
//
angle_t P_PointToAngle(fixed_t xo, fixed_t yo, fixed_t x, fixed_t y)
{	
   x -= xo;
   y -= yo;

   if((x | y) == 0)
      return 0;

   if(x >= 0)
   {
      if (y >= 0)
      {
         if(x > y)
         {
            // octant 0
            return tantoangle[SlopeDiv(y, x)];
         }
         else
         {
            // octant 1
            return ANG90 - 1 - tantoangle[SlopeDiv(x, y)];
         }
      }
      else
      {
         y = -y;

         if(x > y)
         {
            // octant 8
            return 0 - tantoangle[SlopeDiv(y, x)];
         }
         else
         {
            // octant 7
            return ANG270 + tantoangle[SlopeDiv(x, y)];
         }
      }
   }
   else
   {
      x = -x;

      if(y >= 0)
      {
         if(x > y)
         {
            // octant 3
            return ANG180 - 1 - tantoangle[SlopeDiv(y, x)];
         }
         else
         {
            // octant 2
            return ANG90 + tantoangle[SlopeDiv(x, y)];
         }
      }
      else
      {
         y = -y;

         if(x > y)
         {
            // octant 4
            return ANG180 + tantoangle[SlopeDiv(y, x)];
         }
         else
         {
            // octant 5
            return ANG270 - 1 - tantoangle[SlopeDiv(x, y)];
         }
      }
   }

   return 0;
}

//----------------------------------------------------------------------------
//
// $Log: p_maputl.c,v $
// Revision 1.13  1998/05/03  22:16:48  killough
// beautification
//
// Revision 1.12  1998/03/20  00:30:03  phares
// Changed friction to linedef control
//
// Revision 1.11  1998/03/19  14:37:12  killough
// Fix ThingIsOnLine()
//
// Revision 1.10  1998/03/19  00:40:52  killough
// Change ThingIsOnLine() comments
//
// Revision 1.9  1998/03/16  12:34:45  killough
// Add ThingIsOnLine() function
//
// Revision 1.8  1998/03/09  18:27:10  phares
// Fixed bug in neighboring variable friction sectors
//
// Revision 1.7  1998/03/09  07:19:26  killough
// Remove use of FP for point/line queries
//
// Revision 1.6  1998/03/02  12:03:43  killough
// Change blockmap offsets to 32-bit
//
// Revision 1.5  1998/02/23  04:45:24  killough
// Relax blockmap iterator to demo_compatibility
//
// Revision 1.4  1998/02/02  13:41:38  killough
// Fix demo sync programs caused by last change
//
// Revision 1.3  1998/01/30  23:13:10  phares
// Fixed delimiting 0 bug in P_BlockLinesIterator
//
// Revision 1.2  1998/01/26  19:24:11  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------

