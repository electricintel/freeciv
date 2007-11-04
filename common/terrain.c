/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "fcintl.h"
#include "game.h"
#include "map.h"
#include "mem.h"		/* free */
#include "rand.h"
#include "shared.h"
#include "support.h"

#include "terrain.h"

static struct terrain civ_terrains[MAX_NUM_TERRAINS];
static struct resource civ_resources[MAX_NUM_RESOURCES];

enum tile_special_type infrastructure_specials[] = {
  S_ROAD,
  S_RAILROAD,
  S_IRRIGATION,
  S_FARMLAND,
  S_MINE,
  S_FORTRESS,
  S_AIRBASE,
  S_LAST
};

/* T_UNKNOWN isn't allowed here. */
#define SANITY_CHECK_TERRAIN(pterrain)					    \
  assert((pterrain)->index >= 0						    \
	 && (pterrain)->index < game.control.terrain_count		    \
	 && &civ_terrains[(pterrain)->index] == (pterrain))

/****************************************************************************
  Inialize terrain structures.
****************************************************************************/
void terrains_init(void)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(civ_terrains); i++) {
    /* Can't use terrain_by_number here because it does a bounds check. */
    civ_terrains[i].index = i;
  }
  for (i = 0; i < ARRAY_SIZE(civ_resources); i++) {
    civ_resources[i].index = i;
  }
}

/****************************************************************************
  Return the terrain type matching the identifier, or T_UNKNOWN if none matches.
****************************************************************************/
struct terrain *terrain_by_identifier(const char identifier)
{
  if (UNKNOWN_TERRAIN_IDENTIFIER == identifier) {
    return T_UNKNOWN;
  }
  terrain_type_iterate(pterrain) {
    if (pterrain->identifier == identifier) {
      return pterrain;
    }
  } terrain_type_iterate_end;

  return T_UNKNOWN;
}

/****************************************************************************
  Return the terrain for the given terrain index.
****************************************************************************/
struct terrain *terrain_by_number(Terrain_type_id type)
{
  if (type < 0 || type >= game.control.terrain_count) {
    /* This isn't an error; some T_UNKNOWN callers depend on it. */
    return NULL;
  }
  return &civ_terrains[type];
}

/****************************************************************************
  Return the terrain type matching the name, or T_UNKNOWN if none matches.
****************************************************************************/
struct terrain *find_terrain_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  terrain_type_iterate(pterrain) {
    if (0 == mystrcasecmp(terrain_rule_name(pterrain), qname)) {
      return pterrain;
    }
  } terrain_type_iterate_end;

  return T_UNKNOWN;
}

/****************************************************************************
  Return the terrain type matching the name, or T_UNKNOWN if none matches.
****************************************************************************/
struct terrain *find_terrain_by_translated_name(const char *name)
{
  terrain_type_iterate(pterrain) {
    if (0 == strcmp(terrain_name_translation(pterrain), name)) {
      return pterrain;
    }
  } terrain_type_iterate_end;

  return T_UNKNOWN;
}

/****************************************************************************
  Return the (translated) name of the terrain.
  You don't have to free the return pointer.
****************************************************************************/
const char *terrain_name_translation(struct terrain *pterrain)
{
  SANITY_CHECK_TERRAIN(pterrain);
  if (NULL == pterrain->name.translated) {
    /* delayed (unified) translation */
    pterrain->name.translated = ('\0' == pterrain->name.vernacular[0])
				? pterrain->name.vernacular
				: Q_(pterrain->name.vernacular);
  }
  return pterrain->name.translated;
}

/**************************************************************************
  Return the (untranslated) rule name of the terrain.
  You don't have to free the return pointer.
**************************************************************************/
const char *terrain_rule_name(const struct terrain *pterrain)
{
  return Qn_(pterrain->name.vernacular);
}

/****************************************************************************
  Return the terrain flag matching the given string, or TER_LAST if there's
  no match.
****************************************************************************/
enum terrain_flag_id find_terrain_flag_by_rule_name(const char *s)
{
  enum terrain_flag_id flag;
  const char *flag_names[] = {
    /* Must match terrain flags in terrain.h. */
    "NoBarbs",
    "NoPollution",
    "NoCities",
    "Starter",
    "CanHaveRiver",
    "UnsafeCoast",
    "Unsafe",
    "Oceanic"
  };

  assert(ARRAY_SIZE(flag_names) == TER_COUNT);

  for (flag = TER_FIRST; flag < TER_LAST; flag++) {
    if (mystrcasecmp(flag_names[flag], s) == 0) {
      return flag;
    }
  }

  return TER_LAST;
}

/****************************************************************************
  Return a random terrain that has the specified flag.  Returns T_UNKNOWN if
  there is no matching terrain.
  FIXME: currently called only by mapgen.c, move there and check error.
****************************************************************************/
struct terrain *pick_terrain_by_flag(enum terrain_flag_id flag)
{
  bool has_flag[T_COUNT];
  int count = 0;

  terrain_type_iterate(pterrain) {
    if ((has_flag[pterrain->index] = terrain_has_flag(pterrain, flag))) {
      count++;
    }
  } terrain_type_iterate_end;

  count = myrand(count);
  terrain_type_iterate(pterrain) {
    if (has_flag[pterrain->index]) {
      if (count == 0) {
	return pterrain;
      }
      count--;
    }
  } terrain_type_iterate_end;

  die("Reached end of pick_terrain_by_flag!");
  return T_UNKNOWN;
}

/****************************************************************************
  Check for resource in terrain resources list.
****************************************************************************/
bool terrain_has_resource(const struct terrain *pterrain,
			  const struct resource *presource)
{
  struct resource **r = pterrain->resources;

  while (NULL != *r) {
    if (*r == presource) {
      return TRUE;
    }
    r++;
  }
  return FALSE;
}

/****************************************************************************
  Free memory which is associated with terrain types.
****************************************************************************/
void terrains_free(void)
{
  terrain_type_iterate(pterrain) {
    free(pterrain->helptext);
    pterrain->helptext = NULL;
  } terrain_type_iterate_end;
}

/****************************************************************************
  Return the resource for the given resource index.
****************************************************************************/
struct resource *resource_by_number(Resource_type_id type)
{
  if (type < 0 || type >= game.control.resource_count) {
    /* This isn't an error; some callers depend on it. */
    return NULL;
  }
  return &civ_resources[type];
}

/****************************************************************************
  Return the resource type matching the name, or T_UNKNOWN if none matches.
****************************************************************************/
struct resource *find_resource_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  resource_type_iterate(presource) {
    if (0 == strcmp(resource_rule_name(presource), qname)) {
      return presource;
    }
  } resource_type_iterate_end;

  return NULL;
}

/****************************************************************************
  Return the (translated) name of the resource.
  You don't have to free the return pointer.
****************************************************************************/
const char *resource_name_translation(struct resource *presource)
{
  if (NULL == presource->name.translated) {
    /* delayed (unified) translation */
    presource->name.translated = ('\0' == presource->name.vernacular[0])
				 ? presource->name.vernacular
				 : Q_(presource->name.vernacular);
  }
  return presource->name.translated;
}

/**************************************************************************
  Return the (untranslated) rule name of the resource.
  You don't have to free the return pointer.
**************************************************************************/
const char *resource_rule_name(const struct resource *presource)
{
  return Qn_(presource->name.vernacular);
}


/****************************************************************************
  This iterator behaves like adjc_iterate or cardinal_adjc_iterate depending
  on the value of card_only.
****************************************************************************/
#define variable_adjc_iterate(center_tile, _tile, card_only)		\
{									\
  enum direction8 *_tile##_list;					\
  int _tile##_count;							\
									\
  if (card_only) {							\
    _tile##_list = map.cardinal_dirs;					\
    _tile##_count = map.num_cardinal_dirs;				\
  } else {								\
    _tile##_list = map.valid_dirs;					\
    _tile##_count = map.num_valid_dirs;					\
  }									\
  adjc_dirlist_iterate(center_tile, _tile, _tile##_dir,			\
		       _tile##_list, _tile##_count) {

#define variable_adjc_iterate_end					\
  } adjc_dirlist_iterate_end;						\
}


/****************************************************************************
  Returns TRUE iff any adjacent tile contains the given terrain.
****************************************************************************/
bool is_terrain_near_tile(const struct tile *ptile,
			  const struct terrain *pterrain)
{
  adjc_iterate(ptile, adjc_tile) {
    if (pterrain && adjc_tile->terrain == pterrain) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/****************************************************************************
  Return the number of adjacent tiles that have the given terrain.
****************************************************************************/
int count_terrain_near_tile(const struct tile *ptile,
			    bool cardinal_only, bool percentage,
			    const struct terrain *pterrain)
{
  int count = 0, total = 0;

  variable_adjc_iterate(ptile, adjc_tile, cardinal_only) {
    if (pterrain && tile_get_terrain(adjc_tile) == pterrain) {
      count++;
    }
    total++;
  } variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / total;
  }
  return count;
}

/****************************************************************************
  Return the number of adjacent tiles that have the given terrain property.
****************************************************************************/
int count_terrain_property_near_tile(const struct tile *ptile,
				     bool cardinal_only, bool percentage,
				     enum mapgen_terrain_property prop)
{
  int count = 0, total = 0;

  variable_adjc_iterate(ptile, adjc_tile, cardinal_only) {
    if (adjc_tile->terrain->property[prop] > 0) {
      count++;
    }
    total++;
  } variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / total;
  }
  return count;
}

/* Names of specials.
 * (These must correspond to enum tile_special_type.)
 */
static const char *tile_special_type_names[] =
{
  N_("Road"),
  N_("Irrigation"),
  N_("Railroad"),
  N_("Mine"),
  N_("Pollution"),
  N_("Hut"),
  N_("Fortress"),
  N_("River"),
  N_("Farmland"),
  N_("Airbase"),
  N_("Fallout")
};

/****************************************************************************
  Return the special with the given name, or S_LAST.
****************************************************************************/
enum tile_special_type find_special_by_rule_name(const char *name)
{
  assert(ARRAY_SIZE(tile_special_type_names) == S_LAST);

  tile_special_type_iterate(i) {
    if (0 == strcmp(tile_special_type_names[i], name)) {
      return i;
    }
  } tile_special_type_iterate_end;

  return S_LAST;
}

/****************************************************************************
  Return the translated name of the given special.
****************************************************************************/
const char *special_name_translation(enum tile_special_type type)
{
  assert(ARRAY_SIZE(tile_special_type_names) == S_LAST);
  assert(type >= 0 && type < S_LAST);
  return _(tile_special_type_names[type]);
}

/****************************************************************************
  Return the untranslated name of the given special.
****************************************************************************/
const char *special_rule_name(enum tile_special_type type)
{
  assert(ARRAY_SIZE(tile_special_type_names) == S_LAST);
  assert(type >= 0 && type < S_LAST);
  return tile_special_type_names[type];
}

/****************************************************************************
  Add the given special to the set.
****************************************************************************/
void set_special(bv_special *set, enum tile_special_type to_set)
{
  assert(to_set >= 0 && to_set < S_LAST);
  BV_SET(*set, to_set);
}

/****************************************************************************
  Remove the given special from the set.
****************************************************************************/
void clear_special(bv_special *set, enum tile_special_type to_clear)
{
  assert(to_clear >= 0 && to_clear < S_LAST);
  BV_CLR(*set, to_clear);
}

/****************************************************************************
  Clear all specials from the set.
****************************************************************************/
void clear_all_specials(bv_special *set)
{
  BV_CLR_ALL(*set);
}

/****************************************************************************
 Returns TRUE iff the given special is found in the given set.
****************************************************************************/
bool contains_special(bv_special set,
		      enum tile_special_type to_test_for)
{
  assert(to_test_for >= 0 && to_test_for < S_LAST);
  return BV_ISSET(set, to_test_for);
}

/****************************************************************************
  Returns TRUE iff any tile adjacent to (map_x,map_y) has the given special.
****************************************************************************/
bool is_special_near_tile(const struct tile *ptile, enum tile_special_type spe)
{
  adjc_iterate(ptile, adjc_tile) {
    if (tile_has_special(adjc_tile, spe)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/****************************************************************************
  Returns the number of adjacent tiles that have the given map special.
****************************************************************************/
int count_special_near_tile(const struct tile *ptile,
			    bool cardinal_only, bool percentage,
			    enum tile_special_type spe)
{
  int count = 0, total = 0;

  variable_adjc_iterate(ptile, adjc_tile, cardinal_only) {
    if (tile_has_special(adjc_tile, spe)) {
      count++;
    }
    total++;
  } variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / total;
  }
  return count;
}

/****************************************************************************
  Returns TRUE iff any adjacent tile contains terrain with the given flag.
****************************************************************************/
bool is_terrain_flag_near_tile(const struct tile *ptile,
			       enum terrain_flag_id flag)
{
  adjc_iterate(ptile, adjc_tile) {
    struct terrain* pterrain = tile_get_terrain(adjc_tile);
    if (pterrain == NULL) {
      continue;
    }
    
    if (terrain_has_flag(pterrain, flag)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/****************************************************************************
  Return the number of adjacent tiles that have terrain with the given flag.
****************************************************************************/
int count_terrain_flag_near_tile(const struct tile *ptile,
				 bool cardinal_only, bool percentage,
				 enum terrain_flag_id flag)
{
  int count = 0, total = 0;

  variable_adjc_iterate(ptile, adjc_tile, cardinal_only) {
    if (adjc_tile->terrain != T_UNKNOWN
	&& terrain_has_flag(adjc_tile->terrain, flag)) {
      count++;
    }
    total++;
  } variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / total;
  }
  return count;
}

/****************************************************************************
  Return a (static) string with special(s) name(s):
    eg: "Mine"
    eg: "Road/Farmland"
  This only includes "infrastructure", i.e., man-made specials.
****************************************************************************/
const char *get_infrastructure_text(bv_special spe)
{
  static char s[256];
  char *p;
  
  s[0] = '\0';

  /* Since railroad requires road, Road/Railroad is redundant */
  if (contains_special(spe, S_RAILROAD)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Railroad"));
  } else if (contains_special(spe, S_ROAD)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Road"));
  }

  /* Likewise for farmland on irrigation */
  if (contains_special(spe, S_FARMLAND)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Farmland"));
  } else if (contains_special(spe, S_IRRIGATION)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Irrigation"));
  }

  if (contains_special(spe, S_MINE)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Mine"));
  }

  if (contains_special(spe, S_FORTRESS)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Fortress"));
  }

  if (contains_special(spe, S_AIRBASE)) {
    cat_snprintf(s, sizeof(s), "%s/", _("Airbase"));
  }

  p = s + strlen(s) - 1;
  if (*p == '/') {
    *p = '\0';
  }

  return s;
}

/****************************************************************************
  Return the prerequesites needed before building the given infrastructure.
****************************************************************************/
enum tile_special_type get_infrastructure_prereq(enum tile_special_type spe)
{
  if (spe == S_RAILROAD) {
    return S_ROAD;
  } else if (spe == S_FARMLAND) {
    return S_IRRIGATION;
  } else {
    return S_LAST;
  }
}

/****************************************************************************
  Returns the highest-priority (best) infrastructure (man-made special) to
  be pillaged from the terrain set.  May return S_NO_SPECIAL if nothing
  better is available.
****************************************************************************/
enum tile_special_type get_preferred_pillage(bv_special pset)
{
  if (contains_special(pset, S_FARMLAND)) {
    return S_FARMLAND;
  }
  if (contains_special(pset, S_IRRIGATION)) {
    return S_IRRIGATION;
  }
  if (contains_special(pset, S_MINE)) {
    return S_MINE;
  }
  if (contains_special(pset, S_FORTRESS)) {
    return S_FORTRESS;
  }
  if (contains_special(pset, S_AIRBASE)) {
    return S_AIRBASE;
  }
  if (contains_special(pset, S_RAILROAD)) {
    return S_RAILROAD;
  }
  if (contains_special(pset, S_ROAD)) {
    return S_ROAD;
  }
  return S_LAST;
}
