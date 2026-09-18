#!/usr/bin/env python3
"""Dependency check without personal device details, paths or logs."""
import importlib.metadata
import json
import platform
from ios_location_tool import validate_coordinates

if __name__ == "__main__":
    try:
        version = importlib.metadata.version("pymobiledevice3")
    except importlib.metadata.PackageNotFoundError:
        version = "not installed"
    print(json.dumps({"system": platform.system(), "python": platform.python_version(),
                      "pymobiledevice3": version, "coordinate_validation": validate_coordinates(37.3, -122)[0]}, indent=2))
