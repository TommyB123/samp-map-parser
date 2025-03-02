from dataclasses import dataclass
import re

@dataclass
class SampObjectMaterial:
    material_index: int = 0
    material_model: int = 0
    material_txd: str = ""
    material_texture: str = ""
    material_color: str = ""

def parse_material_line(line: str) -> SampObjectMaterial:
    """Extract and return material attributes from a 'SetDynamicObjectMaterial' line."""
    try:
        # Remove the 'SetDynamicObjectMaterial(' part and trailing ')'
        line = re.sub(r'^.*SetDynamicObjectMaterial\(([^,]+),', '', line)
        line = re.sub(r'\);\s*(\w+)?', '', line)
        params = line.split(', ')

        material = SampObjectMaterial(
            material_index=int(params[0]),
            material_model=int(params[1]),
            material_txd=params[2].replace('"', ''),
            material_texture=params[3].replace('"', ''),
            material_color=params[4]
        )

        # sanitize some of the received map data so we don't have to do it when outputting compatible map code
        if (material.material_model == 16644
            and material.material_txd == 'a51_detailstuff'
            and material.material_texture == 'roucghstonebrtb'
        ):
            material.material_model = 18888
            material.material_txd = "forcefields"
            material.material_texture = "white"

        if ' ' in material.material_texture:
            # material has a space in it, replace it with an amazing key word for the map script to identify
            material.material_texture = material.material_texture.replace(' ', 'putafuckingspacehere')

        return material
    except (IndexError, ValueError) as e:
        print(f"Error processing line: {line}\n{e}")
        return None
