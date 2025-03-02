from dataclasses import dataclass

@dataclass
class SampBuildingRemoval:
    remove_modelid: int = 0
    remove_x: float = 0.0
    remove_y: float = 0.0
    remove_z: float = 0.0
    remove_radius: float = 0.0

def parse_building_removal_line(line: str) -> SampBuildingRemoval:
    """Extract and return building removal attributes from a 'RemoveBuildingForPlayer' line."""
    try:
        line = line.strip().replace('RemoveBuildingForPlayer(playerid,', '').replace(');', '')
        params = [param.strip() for param in line.split(',')]
        
        return SampBuildingRemoval(
            remove_modelid=int(params[0]),
            remove_x=float(params[1]),
            remove_y=float(params[2]),
            remove_z=float(params[3]),
            remove_radius=float(params[4])
        )
    except (IndexError, ValueError) as e:
        print(f"Error processing line: {line}\n{e}")
        return None
