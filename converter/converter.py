import argparse
import re 
import unicodedata
from typing import Tuple, List
from pathlib import Path

from parsers.object import parse_dynamic_object_line, SampObject
from parsers.material import parse_material_line
from parsers.material_text import parse_material_text_line
from parsers.removals import parse_building_removal_line, SampBuildingRemoval

path = Path(__file__).parent.parent

parser = argparse.ArgumentParser(description="Convert streamer map files to samp-mapparser format.")
parser.add_argument(
    "-i", "--input",
    help="Input directory containing map files",
    type=str,
    default=str(path / 'map_sources')
)
parser.add_argument(
    "-o", "--output",
    help="Output directory for converted maps",
    type=str,
    default=str(path / 'scriptfiles' / 'maps')
)
parser.add_argument(
    "--input-ext",
    help="Input file extension",
    type=str,
    default=".txt"
)
parser.add_argument(
    "--output-ext",
    help="Output file extension",
    type=str,
    default=".map"
)
parser.add_argument(
    "--file",
    help="Process only a single file instead of directory",
    type=str
)
parser.add_argument(
    "--interior",
    help="Override interior ID for all objects",
    type=int,
    default=None
)
parser.add_argument(
    "--world",
    help="Override world ID for all objects",
    type=int,
    default=None
)
args = parser.parse_args()

input_directory = Path(args.input)
output_directory = Path(args.output)
map_input_file_extension = args.input_ext
map_output_file_extension = args.output_ext


def parse_map(map_input_file: Path) -> Tuple[List[SampBuildingRemoval], List[SampObject]]:
    """Parse a map file and return buildings and objects."""
    try:
        with open(map_input_file, 'r', encoding='utf-8') as file:
            buildings = []
            objects = []
            
            for line_number, line in enumerate(file, 1):
                try:
                    line = line.strip()
                    if not line:
                        continue
                        
                    if 'CreateDynamicObject(' in line:
                        obj = parse_dynamic_object_line(line)
                        if obj:
                            objects.append(obj)
                    elif 'SetDynamicObjectMaterial(' in line:
                        material = parse_material_line(line)
                        if material:
                            objects[-1].materials.append(material)
                    elif 'SetDynamicObjectMaterialText(' in line:
                        text = parse_material_text_line(line)
                        if text:
                            objects[-1].materialtext.append(text)
                    elif 'RemoveBuildingForPlayer(' in line:
                        building = parse_building_removal_line(line)
                        buildings.append(building)
                    else:
                        # Ignore lines that don't need to be parsed
                        continue
                except Exception as e:
                    print(f"Error processing line {line_number}: {e}")
                    
            return buildings, objects
    except Exception as e:
        print(f"Failed to parse map file {map_input_file}: {e}")
        raise

def write_converted_map(map_output_file: Path, buildings, objects):
    try:
        with open(map_output_file, 'a+') as file_output_handler:
            # Write building removal data
            for building in buildings:
                file_output_handler.write(
                    f'rmv {building.remove_modelid} {building.remove_x:.6f} {building.remove_y:.6f} {building.remove_z:.6f} {building.remove_radius:.2f}\n'
                )

            # Write mapped objects
            for obj in objects:
                file_output_handler.write(
                    f'{obj.modelid} {obj.x:.6f} {obj.y:.6f} {obj.z:.6f} {obj.rx:.6f} {obj.ry:.6f} {obj.rz:.6f} {obj.worldid} {obj.interior} {obj.player} {obj.streamdist:.6f} {obj.drawdist:.6f} {obj.areaid} {obj.priority}\n'
                )

                if len(obj.materials) > 0:
                    for material in obj.materials:
                        file_output_handler.write(
                            f'mat {material.material_index} {material.material_model} {material.material_txd} {material.material_texture} {material.material_color}\n'
                        )

                if len(obj.materialtext) > 0:
                    for text in obj.materialtext:
                        file_output_handler.write(
                            f'txt {text.text_index} {text.text_materialsize} {text.text_font} {text.text_fontsize} {text.text_bold} {text.text_fontcolor} {text.text_backgroundcolor} {text.text_alignment} {text.text_contents}\n'
                        )
    except Exception as e:
        print(f"Failed to write converted map to {map_output_file}: {e}")
        raise


def convert_map(map_name: str):
    # Convert map_name to ANSI-compatible format as samp may struggle
    map_name_normalized = unicodedata.normalize('NFKD', map_name).encode('ASCII', 'ignore').decode('ASCII')
    map_name_ansi = re.sub(r'[^\w.-]', '', map_name_normalized.replace(' ', '_'))

    map_input_file = Path(input_directory) / (map_name + map_input_file_extension)
    map_output_file = output_directory / (map_name_ansi + map_output_file_extension)

    # delete output file if it already exists so we don't overwrite or append to converted map data
    if map_output_file.is_file():
        map_output_file.unlink()

    buildings, objects = parse_map(map_input_file)
    
    # Custom parameters if specified
    if args.interior is not None:
        for obj in objects:
            obj.interior = args.interior
    
    if args.world is not None:
        for obj in objects:
            obj.worldid = args.world
    
    output_directory.mkdir(parents=True, exist_ok=True)
    write_converted_map(map_output_file, buildings, objects)
    
    print(f'The converted map has been written to: {map_output_file}')


def convert_dir(directory: Path) -> None:
    """Convert all map files in directory."""
    converted_count = 0
    error_count = 0
    try:
        for file_path in directory.glob(f"*{map_input_file_extension}"):
            print(f'Converting: {file_path.name}')
            try:
                convert_map(file_path.stem)
                converted_count += 1
            except Exception as e:
                print(f"Error converting {file_path.name}: {e}")
                error_count += 1
    except Exception as e:
        print(f"Failed to convert directory {directory}: {e}")
        raise
    finally:
        print(f"Conversion completed. Total maps converted: {converted_count}")
        if error_count > 0:
            print(f"Total errors encountered: {error_count}")

if args.file:
    file_path = Path(args.file)
    if not file_path.is_file():
        print(f"Error: File not found - {file_path}")
    else:
        print(f'Converting single file: {file_path.name}')
        convert_map(file_path.stem)
else:
    convert_dir(input_directory)
