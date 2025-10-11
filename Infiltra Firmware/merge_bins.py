#!/usr/bin/env python3
"""
Universal bin file merger script for all ESP32 devices
Usage: python merge_bins.py [environment_name]
If no environment specified, merges all built environments
"""

import os
import sys
import subprocess
from pathlib import Path

def get_chip_info(env_name):
    """Get chip type and offsets based on environment"""
    # Map environments to chip types
    chip_mapping = {
        "m5stick-c-plus-2": "esp32",
        "m5stick-c-plus-1-1": "esp32", 
        "cardputer": "esp32s3",
        "lilygo-t-embed": "esp32s3",
        "lilygo-cc1101": "esp32"
    }
    
    chip = chip_mapping.get(env_name, "esp32")
    
    # Set offsets based on chip type
    if chip in ["esp32s2", "esp32s3", "esp32c3"]:
        boot_offset = "0x0"
    else:
        boot_offset = "0x1000"
    
    part_offset = "0x8000"
    app_offset = "0x10000"
    
    return chip, boot_offset, part_offset, app_offset

def merge_bins(env_name):
    """Merge bootloader, partitions, and firmware bins"""
    
    # Paths
    build_dir = Path(f".pio/build/{env_name}")
    files_dir = Path("files")
    files_dir.mkdir(exist_ok=True)
    
    # Input files
    bootloader = build_dir / "bootloader.bin"
    partitions = build_dir / "partitions.bin"
    firmware = build_dir / "firmware.bin"
    
    # Output file
    output = files_dir / f"Infiltra-{env_name}.bin"
    
    # Check if input files exist
    for file_path in [bootloader, partitions, firmware]:
        if not file_path.exists():
            print(f"Error: {file_path} not found!")
            return False
    
    # Get chip info
    chip, boot_offset, part_offset, app_offset = get_chip_info(env_name)
    
    # Build esptool command
    cmd = [
        "python", "-m", "esptool",
        "--chip", chip,
        "merge_bin",
        "--output", str(output),
        boot_offset, str(bootloader),
        part_offset, str(partitions), 
        app_offset, str(firmware)
    ]
    
    print(f"Merging bins for {env_name}...")
    print(f"Command: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"Success! Merged bin created: {output}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running esptool: {e}")
        print(f"stdout: {e.stdout}")
        print(f"stderr: {e.stderr}")
        return False
    except FileNotFoundError:
        print("Error: esptool not found. Install with: pip install esptool")
        return False

def get_built_environments():
    """Get list of environments that have been built"""
    build_base = Path(".pio/build")
    if not build_base.exists():
        return []
    
    built_envs = []
    for env_dir in build_base.iterdir():
        if env_dir.is_dir():
            firmware_bin = env_dir / "firmware.bin"
            if firmware_bin.exists():
                built_envs.append(env_dir.name)
    
    return built_envs

def merge_all_environments():
    """Merge bins for all built environments"""
    envs = get_built_environments()
    if not envs:
        print("No built environments found. Run 'pio run' first.")
        return False
    
    success_count = 0
    for env in envs:
        print(f"\n--- Processing {env} ---")
        if merge_bins(env):
            success_count += 1
    
    print(f"\nSummary: {success_count}/{len(envs)} environments merged successfully")
    return success_count == len(envs)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        # No arguments - merge all built environments
        success = merge_all_environments()
    elif len(sys.argv) == 2:
        # Single environment specified
        env_name = sys.argv[1]
        success = merge_bins(env_name)
    else:
        print("Usage: python merge_bins.py [environment_name]")
        print("Examples:")
        print("  python merge_bins.py                  # Merge all built environments")
        print("  python merge_bins.py cardputer        # Merge specific environment")
        sys.exit(1)
    
    sys.exit(0 if success else 1)