import os
path = os.getcwd()


class samp_object_material():
    material_index: int
    material_model: int
    material_txd: str
    material_texture: str
    material_color: str


class samp_object_material_text():
    text_index: int
    text_contents: str
    text_materialsize: int
    text_font: str
    text_fontsize: int
    text_bold: int
    text_fontcolor: str
    text_backgroundcolor: str
    text_alignment: int


class samp_building_removal():
    remove_modelid: int
    remove_x: float
    remove_y: float
    remove_z: float
    remove_radius: float


class samp_object():
    def __init__(self):
        self.materials = []
        self.materialtext = []

    modelid: int
    x: float
    y: float
    z: float
    rx: float
    ry: float
    rz: float
    worldid: int
    interior: int
    player: int
    streamdist: float
    drawdist: float
    areaid: int
    priority: int
    materials: list
    materialtext: list
    isdoor: bool


newworldid = -1
interior = -1
player = -1
streamdist = 300.0
drawdist = 300.0
areaid = -1
priority = 0
mapname = ""


def convert():
    # delete output file if it already exists so we don't overwrite or append to converted map data
    if os.path.isfile(f'{path}/{mapname}') is True:
        os.remove(f'{path}/{mapname}')

    input = open('input.txt', 'r')
    objects = []
    buildings = []

    for line in input:
        if line.find('CreateDynamicObject(') != -1:
            line = line.replace('tmpobjid = CreateDynamicObject(', '')
            line = line.replace('CreateDynamicObject(', '')
            line = line.replace(');\n', '')
            line = line.replace('); \n', '')
            params = line.split(', ')

            object = samp_object()
            object.modelid = int(params[0])
            object.x = float(params[1])
            object.y = float(params[2])
            object.z = float(params[3])
            object.rx = float(params[4])
            object.ry = float(params[5])
            object.rz = float(params[6])
            object.worldid = newworldid if newworldid != -5 else (-1 if params[7] is None else int(params[7]))

            object.interior = interior if len(params) <= 8 else int(params[8])
            object.player = interior if len(params) <= 9 else int(params[9])
            object.streamdist = streamdist if len(params) <= 10 else float(params[10])
            object.drawdist = drawdist if len(params) <= 11 else float(params[11])
            object.areaid = areaid if len(params) <= 12 else int(params[12])
            object.priority = priority if len(params) <= 13 else int(params[13])

            objects.append(object)
        elif line.find('SetDynamicObjectMaterial(') != -1:
            line = line.replace('SetDynamicObjectMaterial(', '')
            line = line.replace(');\n', '')
            line = line.replace('tmpobjid,', '')
            params = line.split(', ')

            material = samp_object_material()
            material.material_index = int(params[0])
            material.material_model = int(params[1])
            material.material_txd = params[2]
            material.material_texture = params[3]
            material.material_color = params[4]

            # remove quotation marks from txd and texture name since the map script doesn't parse them
            material.material_txd = material.material_txd.replace('"', '')
            material.material_texture = material.material_texture.replace('"', '')

            # sanitize some of the received map data so we don't have to do it when outputting compatible map code
            if material.material_model == 16644 and material.material_txd == 'a51_detailstuff' and material.material_texture == 'roucghstonebrtb':
                # this is a transparent texture that doesn't appear correctly inside of interior worlds, we don't want this guy
                material.material_model = 18888
                material.material_txd = "forcefields"
                material.material_texture = "white"

            if material.material_texture.find(' ') != -1:
                # material has a space in it, replace it with an amazing key word for the map script to identify
                material.material_texture = material.material_texture.replace(' ', 'putafuckingspacehere')

            object = objects[len(objects) - 1]
            object.materials.append(material)
        elif line.find('SetDynamicObjectMaterialText(') != -1:
            line = line.replace('SetDynamicObjectMaterialText(', '')
            line = line.replace(');\n', '')
            line = line.replace('tmpobjid,', '')
            params = line.split(', ')

            if len(params) != 9:
                print(f'Expected 9 params when parsing SetDynamicObjectMaterialText but found {len(params)}. Ending conversion.')
                return

            text = samp_object_material_text()
            text.text_index = int(params[0])
            text.text_contents = params[1]
            text.text_materialsize = int(params[2])
            text.text_font = params[3]
            text.text_fontsize = int(params[4])
            text.text_bold = int(params[5])
            text.text_fontcolor = params[6]
            text.text_backgroundcolor = params[7]
            text.text_alignment = int(params[8])

            # sanitize some of the received map data so we don't have to do it when outputting rcrp-compatible map code
            text.text_font = text.text_font.replace(' ', '_')
            text.text_font = text.text_font.replace('"', '')
            text.text_contents = text.text_contents.replace('"', '')
            text.text_contents = text.text_contents.replace('\n', '~N~')

            object = objects[len(objects) - 1]
            object.materialtext.append(text)
        elif line.find('RemoveBuildingForPlayer(') != -1:
            line = line.replace('RemoveBuildingForPlayer(playerid,', '')
            line = line.replace(');\n', '')
            params = line.split(', ')

            building = samp_building_removal()
            building.remove_modelid = int(params[0])
            building.remove_x = float(params[1])
            building.remove_y = float(params[2])
            building.remove_z = float(params[3])
            building.remove_radius = float(params[4])

            buildings.append(building)
        else:  # useless line that doesn't need to be parsed. probably comments or variable assignments that can safely be ignored
            continue
    input.close()

    output = open(mapname, 'a+')
    for building in buildings:
        output.write(f'rmv {building.remove_modelid} {building.remove_x:.6f} {building.remove_y:.6f} {building.remove_z:.6f} {building.remove_radius:.2f}\n')

    for object in objects:
        output.write(f'{object.modelid} {object.x:.6f} {object.y:.6f} {object.z:.6f} {object.rx:.6f} {object.ry:.6f} {object.rz:.6f} {object.worldid} {object.interior} {object.player} {object.streamdist:.6f} {object.drawdist:.6f} {object.areaid} {object.priority}\n')

        if len(object.materials) > 0:
            for material in object.materials:
                output.write(f'mat {material.material_index} {material.material_model} {material.material_txd} {material.material_texture} {material.material_color}\n')

        if len(object.materialtext) > 0:
            for text in object.materialtext:
                output.write(f'txt {text.text_index} {text.text_materialsize} {text.text_font} {text.text_fontsize} {text.text_bold} {text.text_fontcolor} {text.text_backgroundcolor} {text.text_alignment} {text.text_contents}\n')
    output.close()
    print(f'The converted map has been written to {mapname}!')


# make sure input file actually exists before we do anything
if os.path.isfile(f'{path}/input.txt') is False:
    print("Input file doesn't exist.")
    exit()

# make sure the input file isn't empty
if os.stat(f'{path}/input.txt').st_size == 0:
    print("Input file is empty.")
    exit()


mapname = input("Enter a name for the converted map file (be sure to include desired file extension): ") or "output.txt"
if mapname == "output.txt":
    print("No map name supplied. Defaulting to output.txt")
else:
    mapname = f'{mapname}'

newworldid = int(input("Enter the desired world ID of the new map (leave blank for all worlds/the worlds already specified in the input file): ") or -5)
if newworldid < -1:
    print("No map worldid supplied. The map's world IDs will be inherited from the code inside of the input file.")

convert()
