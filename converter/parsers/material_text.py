from dataclasses import dataclass
import re

@dataclass
class SampObjectMaterialText:
    text_index: int = 0
    text_contents: str = ""
    text_materialsize: int = 256
    text_font: str = "Arial"
    text_fontsize: int = 24
    text_bold: int = 1
    text_fontcolor: int = 0xFFFFFFFF
    text_backgroundcolor: int = 0
    text_alignment: int = 0

def parse_material_text_line(line: str) -> SampObjectMaterialText:
    """Extract and return material text attributes from a 'SetDynamicObjectMaterialText' line."""
    try:
        # Remove the 'SetDynamicObjectMaterialText(' part and trailing ')'
        line = re.sub(r'^.*SetDynamicObjectMaterialText\(([^,]+),', '', line)
        line = re.sub(r'\);\s*(\w+)?', '', line)
        params = line.split(', ')

        # Ensure we have at least the required parameters
        if len(params) < 3:
            print(f'Expected at least 3 params when parsing SetDynamicObjectMaterialText but found {len(params)}. Ending conversion.')
            return None

        # Fill in optional parameters with defaults if not provided
        while len(params) < 9:
            params.append('')

        return SampObjectMaterialText(
            text_index=int(params[0]),
            text_contents=params[1].replace('"', '').replace('\n', '~N~'),
            text_materialsize=int(params[2]) if params[2] else 256,
            text_font=params[3].replace(' ', '_').replace('"', '') if params[3] else 'Arial',
            text_fontsize=int(params[4]) if params[4] else 24,
            text_bold=int(params[5]) if params[5] else 1,
            text_fontcolor=int(params[6], 16) if params[6] else 0xFFFFFFFF,
            text_backgroundcolor=int(params[7], 16) if params[7] else 0,
            text_alignment=int(params[8]) if params[8] else 0
        )
    except (IndexError, ValueError) as e:
        print(f"Error processing line: {line}\n{e}")
        return None
