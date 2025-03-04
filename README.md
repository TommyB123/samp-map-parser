# samp-map-parser

[![sampctl](https://img.shields.io/badge/sampctl-samp--map--parser-2f2f2f.svg?style=for-the-badge)](https://github.com/TommyB123/samp-map-parser)

This is a library I wrote for Red County Roleplay to quickly parse individual SA-MP maps on server boot. An easy-to-use API is also provided to manipulate maps (load, unload, reload, etc) during server runtime.

**This library is not very beginner friendly as a result of maps being stored in a non-standard, CSV-like format. It is also rather dependency heavy, so be wary if that's something that bothers you.**

## Dependencies
* [Streamer Plugin](https://github.com/samp-incognito/samp-streamer-plugin)
* [PawnPlus](https://github.com/IllidanS4/PawnPlus)
* [FileManager](https://github.com/JaTochNietDan/SA-MP-FileManager)
* [sscanf2](https://github.com/Y-Less/sscanf)
* [sstrlib](https://github.com/oscar-broman/strlib)

## Installation

Simply install to your project:

```bash
sampctl install TommyB123/samp-map-parser
```

Include in your code and begin using the library:

```pawn
#include <samp-map-parser>
```

## Converting a map
Before you even begin, you'll need a compatible map file. All map data is stored in a CSV-like format, so converting code from the usual SA-MP functions (`CreateObject`, `CreateDynamicObject`) is required in order to load maps with this library. Two conversion scripts are provided as-is, one being a CLI script written in Python, and the other is a SA-MP filterscript. Both of these scripts will quickly convert standard SA-MP map code to parser-compatible code. (I personally recommend using the Python script for converting multiple maps)

**Standard SA-MP object functions are not supported by the conversion scripts. Only streamer plugin functions are.**

The parser (obviously) supports objects, as well as materials, material text and removed buildings. Every argument in `CreateDynamicObject` is supported (except linking to streamer areas), so arbitrary virtual worlds and stream/draw distance can be carried over easily.

Here's an example of what a compatible map file looks like. Two small example files are also present in the `Maps` folder of the repository.

```
rmv 1440 1252.375 247.6125 19.046 7.8313135570295485
rmv 13010 1258.25 245.5156 27.5625 0.25
11504 1252.721191 242.208541 18.554687 0.000000 0.000000 -114.700004 -1 
mat 0 1736 cj_ammo CJ_SLATEDWOOD2 0xFFFFFFFF
mat 1 10101 2notherbuildsfe Bow_Abpave_Gen 0x00000000
mat 2 1736 cj_ammo CJ_CANVAS2 0x00000000
19447 1251.597656 249.466583 20.204685 0.000000 0.000000 -24.700004 -1 
mat 0 12946 ce_bankalley1 sw_brick03 0x00000000
19447 1257.887329 251.823577 20.204685 0.000000 0.000000 65.299995 -1 
mat 0 12946 ce_bankalley1 sw_brick03 0x00000000
19447 1247.573608 240.717590 20.204685 0.000000 0.000000 -24.700004 -1 
mat 0 12946 ce_bankalley1 sw_brick03 0x00000000
1569 1248.763671 243.503890 18.554687 0.000000 0.000000 64.899993 -1 
mat 0 16093 a51_ext des_backdoor1 0xFFFFFFFF
19863 1252.053100 250.648269 21.036880 0.000000 0.000000 65.299995 -1 
mat 0 -1 none none 0xFFFFFFFF
19368 1250.020385 246.661697 16.814670 0.000000 0.000000 -24.999998 -1 
mat 0 19517 noncolored gen_white 0x00000000
19368 1249.118164 248.748794 16.814653 180.000000 180.000000 65.000000 -1 
mat 0 19517 noncolored gen_white 0x00000000
19368 1247.837158 246.002655 16.814653 180.000000 180.000000 65.000000 -1 
mat 0 19517 noncolored gen_white 0x00000000
```

### Using the Python conversion script

#### Prerequisites
* Ensure Python is available on your system. Any modern version (>3.5) should work. The script has been tested with Python 3.8.

#### Steps
1. **Prepare Input File(s):**
   * Place your desired map code into a text file with the `.txt` extension.
   * You can use maps created in [Texture Studio](https://github.com/Pottus/Texture-Studio)
   * By default, the script looks for files in the `map_sources` directory.

2. **Run the Script:**
   * Open a terminal/command line window in the appropriate directory.
   * To convert all files in the `map_sources` directory:
     ```sh
     python converter/converter.py
     ```
   * To convert a single file:
     ```sh
     python converter/converter.py --file path/to/your/input.txt
     ```

#### Script Arguments
* `-i`, `--input`: Input directory containing map files (default: `map_sources`)
* `-o`, `--output`: Output directory for converted maps (default: `scriptfiles/maps`)
* `--input-ext`: Input file extension (default: `.txt`)
* `--output-ext`: Output file extension (default: `.txt`)
* `--file`: Process only a single file instead of a directory
* `--interior`: Override interior ID for all objects
* `--world`: Override world ID for all objects
* `--priority`: Override priority value for all objects

#### Notes
* If the `--file` argument is used, the script will process only the specified file.
* If the `--interior`, `--world`, or `--priority` arguments are provided, they will override the respective attributes for all objects in the map.
* The script will output the converted map files to the specified output directory with the `.txt` extension by default.

### Using the PAWN Filterscript
* Open `mapconvert.pwn` in your editor of choice.
* Paste your map's code into the `CreateMap` function at the top of the script.
* Compile the script and then run it as a SA-MP filterscript.
* If the filterscript ran successfully, your map will be available in the SA-MP server's root directory as `convertedmap.txt`.

## Loading the maps in-game

By default, the script will attempt to parse any `.txt` file stored inside of a `Maps` folder in your server's root directory. These can both be redefined with constants provided below.

To load each file inside of the `Maps` directory, you simply need to run the `ProcessMaps` function.

For example:

```pawn
public OnGameModeInit()
{
    // load all maps on server startup
    ProcessMaps();
}
```

If you just want to cold load maps on server startup without having to worry about recompiling your source code, you don't need to do anything else. However, as alluded to above, there's a handful of extra functions to allow for further control over your server's maps.

For example, you can quickly write up commands for high-level staff to load maps on the fly. 

```pawn
CMD:loadmap(playerid, params[])
{
	if(!IsPlayerAdmin(playerid)) return SysMsg(playerid, "Unauthorized.");
	if(isnull(params)) return SysMsg(playerid, "Usage: /loadmap [map name]");
	if(!IsValidMapFile(params)) return SysMsg(playerid, "Invalid map name.");
	if(GetMapID(params) != INVALID_MAP_ID) return SysMsg(playerid, "The map '%s' is already loaded.", params);

	LoadMap(params);
	SysMsg(playerid, "You have loaded the map '%s' onto the server.", params);
	return true;
}
```

## Functions

| Function | Description |
| - | - |
| `LoadMap(mapname[])` | Attempts to load a file in the map directory using the specified file name. |
| `UnloadMap(mapid)` | Unload a map that is present on the server. (Requires fetching a map's current ID beforehand). All objects associated with the map file are deleted, and if any `RemoveBuildingForPlayer` instructions are contained, they will no longer be applied when new players connect.
| `ReprocessMaps()` | Unloads every single map present on the server then parses every available map file again. |
| `GetMapNameFromID(mapid)` | Resolves a map name from an ID. Returns the map's name as a string. |
| `GetMapIDFromName(const name[])` | Resolves a map's ID from a string. Returns the map ID or `INVALID_MAP_ID` if no map is found. |
| `GetMapIDFromObject(STREAMER_TAG_OBJECT:objectid)` | Resolves a map's ID from a dynamic object ID. Returns the map ID or `INVALID_MAP_ID` if no map is linked to the provided object ID. |
| `GetMapCount()` | Returns the amount of maps currently loaded through the parser script. |
| `GetMapList(String:string, bool:order = false)` | Formats a PawnPlus string with a list of currently loaded maps and how many objects each of them has. `order` dictates whether the list will be sorted by map object count (descending). |
| `IsValidMapFile(const mapname[])` | Checks the map folder for a file with the provided name. Returns `true` if found, `false` if not.
| `ExportMap(mapid)` | Exports a currently-loaded map back to standard SA-MP object code and writes it to a file in the server's root directory. |
| `MoveMapObjects(mapid, Float:xoffset, Float:yoffset, Float:zoffset)` | Temporarily moves the objects belonging to a specific map on the X, Y or Z axis. Not particularly useful, but I needed it for an edge case once or twice. |
| `public OnMapLoaded(mapid, const mapname[], List:objects)` | This callback is triggered whenever a map is loaded.<br />`mapid` is the temporary ID of the map.<br />`mapname` is the name of the map.<br />`List:objects` is a PawnPlus list containing references to every object ID belonging to the map. |

## Relevant constants

| Constant | Default Value | Description  |
| ------------- | ------------- | ------------- |
| `MAP_FILE_DIRECTORY`  | `./Maps/`  | The directory the library will search for map files in.  |
| `MAP_FILE_EXTENSION`  | `txt`  | The file extension that the library will open when parsing map files.  |
| `MAX_MAP_NAME`  | `64`  | The maximum amount of characters the name of a map can contain.  |
| `INVALID_MAP_ID`  | `-1`  | Used to refer to an invalid map ID.  |

## Questions I'll probably get
**Q:** Why does this library use a custom format that requires conversion rather than simply parsing object functions from text files?

**A:** I stole this method from a friend's server, and by the time I realized it was kind of silly, it was too late to go back and I didn't care to rewrite the parsing code. There's a slight speed benefit to this method since there's less repetitive text to parse, which is handy when you're dealing with hundreds of thousands of objects and huge maps with thousands of lines in each file.


**Q:** Why doesn't something work in a way I think would be better?

**A:** I wrote this library to suit my needs and development practices for a server I ran for 8 years. If you have an idea that would improve the library or the conversion scripts, you're more than welcome to toss up a pull request.

## Credits
me
