import re 
from dataclasses import dataclass, field
from typing import List

@dataclass
class SampObject:
    modelid: int = 0
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0
    rx: float = 0.0
    ry: float = 0.0
    rz: float = 0.0
    worldid: int = -1
    interior: int = -1
    player: int = -1
    streamdist: float = 300.0
    drawdist: float = 300.0
    areaid: int = -1
    priority: int = 0
    materials: List = field(default_factory=list)
    materialtext: List = field(default_factory=list)

def parse_dynamic_object_line(line: str, **defaults) -> SampObject:
    """Extract and return attributes from a 'CreateDynamicObject' line."""
    line = re.sub(r'^.*CreateDynamicObject\(|\);\s*(\w+)?', '', line)
    params = [param.strip() for param in line.split(',')]

    try:
        return SampObject(
            modelid=int(params[0]),
            x=float(params[1]),
            y=float(params[2]),
            z=float(params[3]),
            rx=float(params[4]),
            ry=float(params[5]),
            rz=float(params[6]),
            worldid=int(params[7]) if len(params) > 7 else defaults.get('default_worldid', -1),
            interior=int(params[8]) if len(params) > 8 else defaults.get('default_interior', -1),
            player=int(params[9]) if len(params) > 9 else defaults.get('default_player', -1),
            streamdist=float(params[10]) if len(params) > 10 else defaults.get('default_streamdist', 300.0),
            drawdist=float(params[11]) if len(params) > 11 else defaults.get('default_drawdist', 300.0),
            areaid=int(params[12]) if len(params) > 12 else defaults.get('default_areaid', -1),
            priority=int(params[13]) if len(params) > 13 else defaults.get('default_priority', 0)
        )
    except (IndexError, ValueError) as e:
        print(f"Error processing line: {line}\n{e}")
        return None
