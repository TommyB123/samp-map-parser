#if defined __samp_map_parser_included
	#endinput
#endif
#define __samp_map_parser_included

#include <open.mp>
#include <streamer>
#include <PawnPlus>
#include <filemanager>
#include <sscanf2>
#include <strlib>

#if !defined MAX_MAP_NAME
	#define MAX_MAP_NAME (64)
#endif

#define INVALID_MAP_ID -1

#define MAP_OBJ_EXTRA_OFFSET (0)
#define MAP_OBJ_EXTRA_MAPID (1)

#if !defined MAP_FILE_DIRECTORY
	#define MAP_FILE_DIRECTORY "./Maps/"
#endif

#if !defined MAP_FILE_EXTENSION
	#define MAP_FILE_EXTENSION ".txt"
#endif

static enum _:REMOVE_DATA
{
	removemodel,
	Float:removex,
	Float:removey,
	Float:removez,
	Float:removeradius
};

//constants for map data
#define MAP_KEY_NAME "MAP_KEY_NAME"
#define MAP_KEY_OBJECT_LIST "MAP_KEY_OBJECT_LIST"
#define MAP_KEY_REMOVED_BUILDING_LIST "MAP_KEY_REMOVED_BUILDING_LIST"

static Pool:Maps;
static bool:ProcessingMaps;
static bool:SkipMap;

forward OnMapLoaded(mapid, const mapname[], List:objects);
forward OnMapBeginLoading(mapid, const mapname[], firstobjectworldid);

public OnPlayerConnect(playerid)
{
	if(!IsPlayerNPC(playerid)) ProcessRemovedBuildings(playerid);

	#if defined MAP_OnPlayerConnect
		return MAP_OnPlayerConnect(playerid);
	#else
		return 1;
	#endif
}

stock ProcessMaps()
{
	if(!pool_valid(Maps))
	{
		Maps = pool_new(.ordered = true);
	}

	if(!dir_exists(MAP_FILE_DIRECTORY))
	{
		printf("[MAP PARSER] Map directory (%s) was not found. Creating directory and cancelling loading.", MAP_FILE_DIRECTORY);
		ProcessingMaps = false;
		dir_create(MAP_FILE_DIRECTORY);
		return true;
	}

	ProcessingMaps = true;
	new InitialTicks = GetTickCount();

	//read through all map files in the directory and load them
	new dir:MapsDir = dir_open(MAP_FILE_DIRECTORY), fname[MAX_MAP_NAME], ftype;
	while(dir_list(MapsDir, fname, ftype))
	{
		if(ftype == FM_DIR) continue;
		if(strfind(fname, MAP_FILE_EXTENSION) == -1) continue;

		LoadMap(fname);
	}
	dir_close(MapsDir);

	new mapcount = pool_size(Maps);
	printf("[MAP PARSER] Successfully loaded %i %s. [%ims, %i objects, %i removed buildings]\n", mapcount, mapcount == 1 ? "map" : "maps", GetTickCount() - InitialTicks, CountMapObjects(), CountRemovedBuildings());
	ProcessingMaps = false;
	return true;
}

stock LoadMap(mapname[])
{
	if(strlen(mapname) > MAX_MAP_NAME)
	{
		printf("[MAP PARSER] Map name '%s' is too long.", mapname);
		return INVALID_MAP_ID;
	}

	new Map:data = map_new(), name[MAX_MAP_NAME];
	format(name, MAX_MAP_NAME, mapname);
	new index = strfind(mapname, MAP_FILE_EXTENSION);
	if(index != -1)
	{
		name[index] = EOS; //trimn the file extension from the map name
	}
	map_str_add_str(data, MAP_KEY_NAME, name);

	new mapid = GetMapIDFromName(name);
	if(mapid != INVALID_MAP_ID)
	{
		map_delete(data);
		printf("[MAP PARSER] Attempted to load map '%s' while it was already loaded.", mapname);
		return mapid;
	}

	if(!pool_valid(Maps))
	{
		Maps = pool_new();
		pool_set_ordered(Maps, true);
	}

	new List:objects = list_new();
	map_str_add(data, MAP_KEY_OBJECT_LIST, objects);

	new List:buildings = list_new();
	map_str_add(data, MAP_KEY_REMOVED_BUILDING_LIST, buildings);

	new mapindex = pool_add(Maps, data);

	//string for formatting
	new templine[256];

	//file variables
	format(templine, sizeof(templine), "%s%s%s", MAP_FILE_DIRECTORY, name, MAP_FILE_EXTENSION);
	new File:f = f_open(templine, "r");

	//object variables
	new STREAMER_TAG_OBJECT:tempobj, model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, world, interior, playerid, Float:streamdist, Float:drawdist, areaid, priority;

	//object material variables
	new matindex, matmodel, mattxd[64], mattexture[64], matcolor;

	//object material text variables
	new mattext[256], matsize, matfont[32], matfontsize, matbold, matbgcolor, matalign;

	//streamer extra ID array
	new mapobject[2];

	//building removal data
	new remove[REMOVE_DATA];

	while(f_read(f, templine))
	{
		if(!templine[0]) continue;

		if(SkipMap)
		{
			SkipMap = false;
			UnloadMap(mapindex);
			f_close(f);
			return INVALID_MAP_ID;
		}

		if(templine[0] == 'r')
		{
			sscanf(templine, "{s[5]}iffff", remove[removemodel], remove[removex], remove[removey], remove[removez], remove[removeradius]);
			#if defined CA_RemoveBuilding
				CA_RemoveBuilding(remove[removemodel], remove[removex], remove[removey], remove[removez], remove[removeradius]);
			#endif
			list_add_arr(buildings, remove);
			continue;
		}
		else if(templine[0] == 'm')
		{
			sscanf(templine, "{s[5]}iis[64]s[64]N(0)", matindex, matmodel, mattxd, mattexture, matcolor);
			strreplace(mattexture, "putafuckingspacehere", " ", true);
			SetDynamicObjectMaterial(tempobj, matindex, matmodel, mattxd, mattexture, matcolor);
			continue;
		}
		else if(templine[0] == 't')
		{
			sscanf(templine, "{s[5]}iis[32]iinnis[256]", matindex, matsize, matfont, matfontsize, matbold, matcolor, matbgcolor, matalign, mattext);
			strreplace(matfont, "_", " ");
			strreplace(mattext, "~N~", "\n", true);
			strreplace(mattext, "putafuckingspacehere", " ", true);
			SetDynamicObjectMaterialText(tempobj, matindex, mattext, matsize, matfont, matfontsize, matbold, matcolor, matbgcolor, matalign);
			continue;
		}

		//parse an object
		sscanf(templine, "iffffffiI(-1)I(-1)F(300.0)F(300.0)I(-1)I(0)", model, x, y, z, rx, ry, rz, world, interior, playerid, streamdist, drawdist, areaid, priority);
		if(!list_size(objects))
		{
			//call OnMapBeginLoading before the first object is created
			pawn_call_public("OnMapBeginLoading", "isi", mapid, mapname, world);
		}
		tempobj = CreateDynamicObject(model, x, y, z, rx, ry, rz, world, interior, playerid, streamdist, drawdist, STREAMER_TAG_AREA:areaid, priority);

		mapobject[MAP_OBJ_EXTRA_OFFSET] = -5;
		mapobject[MAP_OBJ_EXTRA_MAPID] = mapindex;
		Streamer_SetArrayData(STREAMER_TYPE_OBJECT, tempobj, E_STREAMER_EXTRA_ID, mapobject);

		list_add(objects, _:tempobj);
	}
	f_close(f);

	if(!list_size(objects))
	{
		printf("[MAP PARSER] %s did not contain any valid objects.", mapname);
		pool_remove_deep(Maps, mapindex);
		return INVALID_MAP_ID;
	}

	if(!list_size(buildings))
	{
		map_str_remove_deep(data, MAP_KEY_REMOVED_BUILDING_LIST);
	}

	if(!ProcessingMaps)
	{
		RemoveNewBuildings(mapindex);
	}
	pawn_call_public("OnMapLoaded", "isi", mapindex, mapname, objects);

	return mapindex;
}

stock UnloadMap(mapid)
{
	if(!pool_valid(Maps)) return true;
	if(mapid < 0 || !pool_has(Maps, mapid)) return true;

	new Map:data = Map:pool_get(Maps, mapid), List:objects = List:map_str_get(data, MAP_KEY_OBJECT_LIST);
	if(list_valid(objects) && list_size(objects))
	{
		for(new Iter:i = list_iter(objects); iter_inside(i); iter_move_next(i))
		{
			DestroyDynamicObject(STREAMER_TAG_OBJECT:iter_get(i));
		}
	}

	pool_remove_deep(Maps, mapid);
	return true;
}

stock ReprocessMaps()
{
	if(pool_valid(Maps) && pool_size(Maps))
	{
		for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_erase_deep(i))
		{
			new Map:data = Map:iter_get(i);
			new List:objects = List:map_str_get(data, MAP_KEY_OBJECT_LIST);
			for(new Iter:m = list_iter(objects); iter_inside(m); iter_move_next(m))
			{
				DestroyDynamicObject(STREAMER_TAG_OBJECT:iter_get(m));
			}
		}
	}

	ProcessMaps();
	return true;
}

stock GetMapNameFromID(mapid)
{
	new mapname[MAX_MAP_NAME];
	if(!pool_valid(Maps) || mapid < 0 || !pool_has(Maps, mapid))
	{
		format(mapname, sizeof(mapname), "Invalid map.");
	}
	else
	{
		new Map:data = Map:pool_get(Maps, mapid);
		map_str_get_str(data, MAP_KEY_NAME, mapname);
	}
	return mapname;
}

stock GetMapIDFromName(const name[])
{
	if(!pool_valid(Maps))
	{
		return INVALID_MAP_ID;
	}

	new mapid;
	if(!sscanf(name, "i", mapid))
	{
		if(mapid >= 0 && pool_has(Maps, mapid))
		{
			return mapid;
		}
		else return INVALID_MAP_ID;
	}
	else
	{
		for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_move_next(i))
		{
			new Map:data = Map:iter_get(i), mapname[MAX_MAP_NAME];
			map_str_get_str(data, MAP_KEY_NAME, mapname);
			if(!strcmp(name, mapname, true))
			{
				return iter_get_key(i);
			}
		}
	}
	return INVALID_MAP_ID;
}

stock GetMapIDFromObject(STREAMER_TAG_OBJECT:objectid)
{
	if(!IsValidDynamicObject(objectid)) return INVALID_MAP_ID;

	new mapobject[2];
	Streamer_GetArrayData(STREAMER_TYPE_OBJECT, objectid, E_STREAMER_EXTRA_ID, mapobject);
	if(mapobject[MAP_OBJ_EXTRA_OFFSET] == -5)
	{
		return mapobject[MAP_OBJ_EXTRA_MAPID];
	}
	return INVALID_MAP_ID;
}

stock GetMapCount()
{
	if(pool_valid(Maps))
	{
		return pool_size(Maps);
	}
	else
	{
		return -1;
	}
}

stock GetMapList(String:string, bool:order = false)
{
	if(!pool_valid(Maps))
	{
		print("[MAP PARSER] Could not format map list because no maps are loaded.");
		return false;
	}

	str_append_format(string, "Slot\tMap Name\tObjects\n");
	if(order)
	{
		new mapinfo[2], List:maplist = list_new();
		for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_move_next(i))
		{
			new Map:data = Map:iter_get(i);
			mapinfo[0] = iter_get_key(i);
			mapinfo[1] = list_size(List:map_str_get(data, MAP_KEY_OBJECT_LIST));
			list_add_arr(maplist, mapinfo);
		}

		list_sort(maplist, 1, -1, true);
		for(new Iter:i = list_iter(maplist); iter_inside(i); iter_move_next(i))
		{
			iter_get_arr(i, mapinfo);
			str_append_format(string, "%i\t%s\t%i\n", mapinfo[0], GetMapNameFromID(mapinfo[0]), mapinfo[1]);
		}
		list_delete(maplist);
	}
	else
	{
		for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_move_next(i))
		{
			new Map:data = Map:iter_get(i), mapname[MAX_MAP_NAME], List:objects;
			map_str_get_str(data, MAP_KEY_NAME, mapname);
			objects = List:map_str_get(data, MAP_KEY_OBJECT_LIST);
			str_append_format(string, "%i\t%s\t%i\n", iter_get_key(i), mapname, list_size(objects));
		}
	}
	return true;
}

stock IsValidMapFile(const mapname[])
{
	new file[256];
	format(file, sizeof(file), "%s%s%s", MAP_FILE_DIRECTORY, mapname, MAP_FILE_EXTENSION);
	return (file_exists(file));
}

stock ExportMap(mapid)
{
	if(!pool_valid(Maps))
	{
		printf("[MAP PARSER] Unable to export map %i due to no maps currently being loaded.", mapid);
		return false;
	}

	if(!pool_has(Maps, mapid))
	{
		printf("[MAP PARSER] Unable to export map ID %i. Invalid ID.", mapid);
		return false;
	}

	new mapname[MAX_MAP_NAME], templine[256];
	format(mapname, MAX_MAP_NAME, "%s%s", GetMapNameFromID(mapid), MAP_FILE_EXTENSION);
	if(file_exists(mapname)) file_delete(mapname);
	file_create(mapname);

	new STREAMER_TAG_OBJECT:tempobject, File:f = f_open(mapname, "w"), remove[REMOVE_DATA], Map:data;
	data = Map:pool_get(Maps, mapid);

	f_write(f, "new tempobject;\n");

	//write removed buildings
	if(map_has_str_key(data, MAP_KEY_REMOVED_BUILDING_LIST))
	{
		new List:buildings = List:map_str_get(data, MAP_KEY_REMOVED_BUILDING_LIST);
		for(new Iter:i = list_iter(buildings); iter_inside(i); iter_move_next(i))
		{
			iter_get_arr(i, remove);
			format(templine, sizeof(templine), "RemoveBuildingForPlayer(playerid, %i, %f, %f, %f, %.2f);\n", remove[removemodel], remove[removex], remove[removey], remove[removez], remove[removeradius]);
			f_write(f, templine);
		}
	}

	//write all objects
	for(new k = 0; k < 2; k++) //write objects with materials first, then objects without
	{
		for(new Iter:i = list_iter(List:map_str_get(data, MAP_KEY_OBJECT_LIST)); iter_inside(i); iter_move_next(i))
		{
			tempobject = STREAMER_TAG_OBJECT:iter_get(i);
			if(!IsValidDynamicObject(tempobject)) continue;

			new bool:matused;
			for(new idx = 0; idx < 16; idx++)
			{
				if(IsDynamicObjectMaterialUsed(tempobject, idx) || IsDynamicObjectMaterialTextUsed(tempobject, idx))
				{
					matused = true;
					break;
				}
			}

			new model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, world, interior, player, Float:streamdist, Float:drawdist;
			GetDynamicObjectPos(tempobject, x, y, z);
			GetDynamicObjectRot(tempobject, rx, ry, rz);
			model = Streamer_GetIntData(STREAMER_TYPE_OBJECT, tempobject, E_STREAMER_MODEL_ID);
			world = Streamer_GetIntData(STREAMER_TYPE_OBJECT, tempobject, E_STREAMER_WORLD_ID);
			interior = Streamer_GetIntData(STREAMER_TYPE_OBJECT, tempobject, E_STREAMER_INTERIOR_ID);
			player = Streamer_GetIntData(STREAMER_TYPE_OBJECT, tempobject, E_STREAMER_PLAYER_ID);
			Streamer_GetFloatData(STREAMER_TYPE_OBJECT, tempobject, E_STREAMER_STREAM_DISTANCE, streamdist);
			Streamer_GetFloatData(STREAMER_TYPE_OBJECT, tempobject, E_STREAMER_DRAW_DISTANCE, drawdist);

			if(matused)
			{
				if(k != 0) continue;
				format(templine, sizeof(templine), "%sCreateDynamicObject(%i, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %i, %i, %i, %.2f, %.2f);\n",
					matused ? "tempobject = " : "", model, x, y, z, rx, ry, rz, world, interior, player, streamdist, drawdist);
				f_write(f, templine);
				for(new idx = 0; idx < 16; idx++)
				{
					if(IsDynamicObjectMaterialUsed(tempobject, idx))
					{
						new material_model, material_txd[64], material_texture[64], material_color;
						GetDynamicObjectMaterial(tempobject, idx, material_model, material_txd, material_texture, material_color);

						format(templine, sizeof(templine), "SetDynamicObjectMaterial(tempobject, %i, %i, \"%s\", \"%s\", 0x%08x);\n", idx, material_model, material_txd, material_texture, material_color);
						f_write(f, templine);
					}
					if(IsDynamicObjectMaterialTextUsed(tempobject, idx))
					{
						new material_text[256], material_size, material_font[32], material_fontsize, material_bold, material_fontcolor, material_bgcolor, material_textalign;
						GetDynamicObjectMaterialText(tempobject, idx, material_text, material_size, material_font, material_fontsize, material_bold, material_fontcolor, material_bgcolor, material_textalign);
						strreplace(material_text, "\n", "~N~");

						format(templine, sizeof(templine), "SetDynamicObjectMaterialText(tempobject, %i, \"%s\", %i, \"%s\", %i, %i, 0x%08x, 0x%08x, %i);\n",
							idx, material_text, material_size, material_font, material_fontsize, material_bold, material_fontcolor, material_bgcolor, material_textalign);
						f_write(f, templine);
					}
				}
			}
			if(k == 1)
			{
				format(templine, sizeof(templine), "%sCreateDynamicObject(%i, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %i, %i, %i, %.2f, %.2f);\n",
					matused ? "tempobject = " : "", model, x, y, z, rx, ry, rz, world, interior, player, streamdist, drawdist);
				f_write(f, templine);
			}
		}
	}

	f_close(f);
	return true;
}

stock MoveMapObjects(mapid, Float:xoffset, Float:yoffset, Float:zoffset)
{
	if(!pool_valid(Maps) || !pool_size(Maps) || mapid < 0 || !pool_has(Maps, mapid)) return false;


	new Map:data = Map:pool_get(Maps, mapid);
	for(new Iter:i = list_iter(List:map_str_get(data, MAP_KEY_OBJECT_LIST)), Float:x, Float:y, Float:z, STREAMER_TAG_OBJECT:objectid; iter_inside(i); iter_move_next(i))
	{
		objectid = STREAMER_TAG_OBJECT:iter_get(i);
		if(!IsValidDynamicObject(objectid)) continue;

		GetDynamicObjectPos(objectid, x, y, z);
		SetDynamicObjectPos(objectid, x + xoffset, y + yoffset, z + zoffset);
	}
	return true;
}

static stock RemoveNewBuildings(mapid)
{
	if(pool_valid(Maps) && mapid >= 0 && pool_has(Maps, mapid))
	{
		new Map:data = Map:pool_get(Maps, mapid);
		if(map_has_str_key(data, MAP_KEY_REMOVED_BUILDING_LIST))
		{
			new List:buildings, players[MAX_PLAYERS];
			new playercount = GetPlayers(players);
			buildings = List:map_str_get(data, MAP_KEY_REMOVED_BUILDING_LIST);
			for(new i = 0; i < playercount; ++i)
			{
				RemoveMapBuildingsForPlayer(players[i], buildings);
			}
		}
	}
	return true;
}

static stock ProcessRemovedBuildings(playerid)
{
	if(!pool_valid(Maps) || !pool_size(Maps)) return false;

	for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_move_next(i))
	{
		new Map:data = Map:iter_get(i);
		if(map_has_str_key(data, MAP_KEY_REMOVED_BUILDING_LIST))
		{
			new List:buildings = List:map_str_get(data, MAP_KEY_REMOVED_BUILDING_LIST);
			RemoveMapBuildingsForPlayer(playerid, buildings);
		}
	}
	return true;
}

static stock RemoveMapBuildingsForPlayer(playerid, List:buildings)
{
	if(!list_valid(buildings)) return false;

	for(new Iter:i = list_iter(buildings), remove[REMOVE_DATA]; iter_inside(i); iter_move_next(i))
	{
		iter_get_arr(i, remove);
		RemoveBuildingForPlayer(playerid, remove[removemodel], remove[removex], remove[removey], remove[removez], remove[removeradius]);
	}
	return true;
}

static stock CountMapObjects()
{
	new count;
	if(pool_valid(Maps) && pool_size(Maps))
	{
		for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_move_next(i))
		{
			new Map:data = Map:iter_get(i);
			count += list_size(List:map_str_get(data, MAP_KEY_OBJECT_LIST));
		}
	}
	return count;
}

static stock CountRemovedBuildings()
{
	new count;
	if(pool_valid(Maps) && pool_size(Maps))
	{
		for(new Iter:i = pool_iter(Maps); iter_inside(i); iter_move_next(i))
		{
			new Map:data = Map:iter_get(i);
			if(map_has_str_key(data, MAP_KEY_REMOVED_BUILDING_LIST))
			{
				new List:buildings = List:map_str_get(data, MAP_KEY_REMOVED_BUILDING_LIST);
				if(list_valid(buildings))
				{
					count += list_size(buildings);
				}
			}
		}
	}
	return count;
}

stock IsMapParserRunning()
{
	return ProcessingMaps;
}

stock SkipCurrentlyLoadingMap()
{
	SkipMap = true;
}

//hooks
#if defined _ALS_OnPlayerConnect
	#undef OnPlayerConnect
#else
	#define _ALS_OnPlayerConnect
#endif

#define OnPlayerConnect MAP_OnPlayerConnect
#if defined MAP_OnPlayerConnect
	forward MAP_OnPlayerConnect(playerid);
#endif
