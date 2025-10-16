#!/usr/bin/env python3
"""
TIGR SD Card Data Extractor
Extracts raw sector data from SD card and converts to CSV
"""

import sys
import os
import struct
import webbrowser

def find_sd_card():
    """Find SD card device (cross-platform)"""
    if sys.platform == 'win32':
        # Windows: list physical drives
        print("Windows detected. Please identify your SD card:")
        print("  Open Disk Management to find drive number")
        print("  Example: \\\\.\\PhysicalDrive1")
        drive = input("\nEnter SD card path: ").strip()
        return drive
    elif sys.platform == 'darwin':
        # macOS
        print("macOS detected. Run: diskutil list")
        print("Look for your SD card (usually /dev/disk2 or /dev/disk3)")
        drive = input("\nEnter SD card path (e.g., /dev/disk2): ").strip()
        return drive
    else:
        # Linux
        print("Linux detected. Run: lsblk")
        print("Look for your SD card (usually /dev/sdb or /dev/mmcblk0)")
        drive = input("\nEnter SD card path (e.g., /dev/sdb): ").strip()
        return drive

def extract_data(device_path, output_file='tigr_data.csv', max_sectors=1000):
    """Extract CSV data from raw sectors"""
    
    print(f"\n📀 Reading from: {device_path}")
    print(f"📊 Output file: {output_file}")
    print(f"⚠️  Reading up to {max_sectors} sectors (512 KB)")
    
    try:
        # Open device for reading (requires admin/sudo)
        with open(device_path, 'rb') as sd_card:
            # Read sectors
            data = sd_card.read(max_sectors * 512)  # 512 bytes per sector
            
            print(f"✓ Read {len(data)} bytes")
            
            # Convert bytes to text (remove null bytes)
            text = data.decode('ascii', errors='ignore')
            text = text.replace('\x00', '')  # Remove null padding
            
            # Find CSV data (starts with "Muon#,Band")
            if "Muon#,Band" not in text:
                print("❌ No TIGR data found! Card may be empty or corrupted.")
                return False
            
            # Extract only CSV portion
            start_idx = text.find("Muon#,Band")
            csv_data = text[start_idx:]
            
            # Remove any garbage after last valid line
            lines = csv_data.split('\n')
            valid_lines = []
            for line in lines:
                if ',' in line and line.strip():
                    # Check if line looks like valid CSV
                    parts = line.split(',')
                    if len(parts) >= 5:
                        valid_lines.append(line)
                    elif 'Muon#' in line:  # Header
                        valid_lines.append(line)
            
            # Write to output file
            with open(output_file, 'w') as f:
                f.write('\n'.join(valid_lines))
            
            print(f"\n✅ Success! Extracted {len(valid_lines)-1} readings")
            print(f"📁 Saved to: {output_file}")
            
            return True
            
    except PermissionError:
        print("\n❌ Permission denied!")
        if sys.platform == 'win32':
            print("   Run this script as Administrator")
        else:
            print("   Run with: sudo python3 extract_tigr_data.py")
        return False
    except FileNotFoundError:
        print(f"\n❌ Device not found: {device_path}")
        print("   Double-check the device path")
        return False
    except Exception as e:
        print(f"\n❌ Error: {e}")
        return False

def open_analyzer(csv_file):
    """Open the web analyzer automatically"""
    analyzer_path = "tigr_analyzer.html"
    
    if os.path.exists(analyzer_path):
        # Open analyzer in browser
        abs_path = os.path.abspath(analyzer_path)
        webbrowser.open(f'file://{abs_path}')
        print(f"\n🚀 Opening TIGR Analyzer...")
        print(f"   Upload {csv_file} to visualize your data!")
    else:
        print(f"\n⚠️  tigr_analyzer.html not found in current directory")
        print(f"   Manually open the analyzer and upload {csv_file}")

def main():
    print("=" * 60)
    print("  TIGR SD Card Data Extractor")
    print("=" * 60)
    
    # Check if running with admin/sudo
    if sys.platform != 'win32' and os.geteuid() != 0:
        print("\n⚠️  This script needs root access to read raw devices")
        print("   Please run: sudo python3 extract_tigr_data.py\n")
        sys.exit(1)
    
    # Find SD card
    print("\n🔍 Step 1: Identify your SD card")
    device = find_sd_card()
    
    if not device:
        print("❌ No device specified")
        sys.exit(1)
    
    # Confirm before reading
    print(f"\n⚠️  About to read from: {device}")
    confirm = input("Continue? (yes/no): ").strip().lower()
    
    if confirm != 'yes':
        print("Cancelled.")
        sys.exit(0)
    
    # Extract data
    print("\n📊 Step 2: Extracting data...")
    output_file = 'tigr_data.csv'
    
    if extract_data(device, output_file):
        # Open analyzer
        print("\n🎉 Step 3: Opening analyzer...")
        open_analyzer(output_file)
    else:
        print("\n❌ Extraction failed")
        sys.exit(1)
    
    print("\n✅ Done!")

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Cancelled by user")
        sys.exit(0)