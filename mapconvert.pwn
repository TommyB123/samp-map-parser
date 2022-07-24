#define FILTERSCRIPT

#include <a_samp>
#include <streamer>
#include <strlib>
#include <filemanager>

CreateMap()
{
    //put your map code here, compile the script and then launch the SA-MP server with this filterscript loaded.
    //the map you input will be written to convertedmap.txt in a format compatible with the parser.
}

public OnFilterScriptInit()
{
	new str[2056];

	//create the map objects.
	CreateMap();

	//if a converted map file already exists, delete it. created another no matter what
	if(file_exists("convertedmap.txt")) file_delete("convertedmap.txt");
	file_create("convertedmap.txt");

	//open the file
	new File:handle = f_open("convertedmap.txt", "w");

	new model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, world, InitialTicks = GetTickCount();
	for(new i = 1; i <= Streamer_GetUpperBound(STREAMER_TYPE_OBJECT); i++)
	{
		if(!IsValidDynamicObject(i)) continue;

		//grab the object model, world and position
		model = Streamer_GetIntData(STREAMER_TYPE_OBJECT, i, E_STREAMER_MODEL_ID);
		world = Streamer_GetIntData(STREAMER_TYPE_OBJECT, i, E_STREAMER_WORLD_ID);
		GetDynamicObjectPos(i, x, y, z);
		GetDynamicObjectRot(i, rx, ry, rz);

		//print the object data
		format(str, sizeof(str), "%i %f %f %f %f %f %f %i\n", model, x, y, z, rx, ry, rz, world);
		f_write(handle, str);

		//check object for any materials, print them if they exist
		for(new m = 0; m < 15; m++)
		{
			if(IsDynamicObjectMaterialUsed(i, m))
			{
				new tmodel, txdname[64], texturename[64], materialcolor;
				GetDynamicObjectMaterial(i, m, tmodel, txdname, texturename, materialcolor);

				//always replace this texture because it's shit and doesn't work in interiors
				if(tmodel == 16644 && !strcmp(txdname, "a51_detailstuff") && !strcmp(texturename, "roucghstonebrtb"))
				{
					tmodel = 18888;
					txdname = "forcefields";
					texturename = "white";
				}
				if(strfind(texturename, " ") != -1) strreplace(texturename, " ", "putafuckingspacehere");

				format(str, sizeof(str), "mat %i %i %s %s 0x%08x\n", m, tmodel, txdname, texturename, materialcolor);
				f_write(handle, str);
			}
		}

		//check for any material texts, print them if they exist
		for(new m = 0; m < 15; m++)
		{
			if(IsDynamicObjectMaterialTextUsed(i, m))
			{
				new text[128], materialsize, fontface[32], fontsize, bold, fontcolor, backcolor, alignment;
				GetDynamicObjectMaterialText(i, m, text, materialsize, fontface, fontsize, bold, fontcolor, backcolor, alignment);

				strreplace(fontface, " ", "_");
				strreplace(text, "\n", "~N~");
				format(str, sizeof(str), "txt %i %i %s %i %i 0x%x 0x%08x %i %s\n", m, materialsize, fontface, fontsize, bold, fontcolor, backcolor, alignment, text);
				f_write(handle, str);
			}
		}
	}
	if(Streamer_GetUpperBound(STREAMER_TYPE_OBJECT) != 1) printf("Map exported to convertedmap.txt in %i milliseconds.", GetTickCount() - InitialTicks);
	f_close(handle);
}