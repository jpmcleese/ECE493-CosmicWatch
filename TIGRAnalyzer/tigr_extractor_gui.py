#!/usr/bin/env python3
"""
TIGR SD Card Data Extractor - GUI Version
User-friendly interface for extracting muon detection data
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess
import sys
import os
import webbrowser
import ctypes

class TIGRExtractorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("TIGR SD Card Data Extractor")
        self.root.geometry("600x500")
        self.root.resizable(False, False)
        
        # Check admin rights
        self.is_admin = self.check_admin()
        
        # Initialize drive mapping
        self.drive_map = {}
        
        self.create_widgets()
        self.populate_drives()
    
    def check_admin(self):
        """Check if running with administrator privileges"""
        try:
            return ctypes.windll.shell32.IsUserAnAdmin()
        except:
            return False
    
    def create_widgets(self):
        """Create the GUI interface"""
        
        # Header
        header_frame = tk.Frame(self.root, bg="#4d66ad", height=80)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        title_label = tk.Label(
            header_frame, 
            text="TIGR Data Extractor",
            font=("Dubai", 20, "bold"),
            bg="#4d66ad",
            fg="white"
        )
        title_label.pack(pady=20)
        
        # Main content
        main_frame = tk.Frame(self.root, padx=30, pady=20)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Admin status
        if not self.is_admin:
            warning_frame = tk.Frame(main_frame, bg="#fef3c7", relief=tk.RAISED, bd=2)
            warning_frame.pack(fill=tk.X, pady=(0, 20))
            
            warning_icon = tk.Label(warning_frame, text="⚠️", bg="#fef3c7", font=("Dubai", 16))
            warning_icon.pack(side=tk.LEFT, padx=10, pady=10)
            
            warning_text = tk.Label(
                warning_frame,
                text="Not running as Administrator!\nClick 'Request Admin' to elevate privileges.",
                bg="#fef3c7",
                fg="#92400e",
                font=("Dubai", 10),
                justify=tk.LEFT
            )
            warning_text.pack(side=tk.LEFT, pady=10)
            
            admin_btn = tk.Button(
                warning_frame,
                text="Request Admin",
                command=self.request_admin,
                bg="#f59e0b",
                fg="white",
                font=("Dubai", 10, "bold"),
                relief=tk.RAISED,
                cursor="hand2"
            )
            admin_btn.pack(side=tk.RIGHT, padx=10, pady=10)
        else:
            admin_frame = tk.Frame(main_frame, bg="#d1fae5", relief=tk.RAISED, bd=2)
            admin_frame.pack(fill=tk.X, pady=(0, 20))
            
            admin_label = tk.Label(
                admin_frame,
                text="Running as Administrator",
                bg="#d1fae5",
                fg="#065f46",
                font=("Dubai", 10, "bold")
            )
            admin_label.pack(pady=10)
        
        # Step 1: Select Drive
        step1_label = tk.Label(
            main_frame,
            text="Select SD Card Drive",
            font=("Dubai", 12, "bold")
        )
        step1_label.pack(anchor=tk.W, pady=(10, 5))
        
        drive_frame = tk.Frame(main_frame)
        drive_frame.pack(fill=tk.X, pady=(0, 20))
        
        self.drive_var = tk.StringVar()
        self.drive_combo = ttk.Combobox(
            drive_frame,
            textvariable=self.drive_var,
            font=("Dubai", 10),
            state="readonly",
            width=40
        )
        self.drive_combo.pack(side=tk.LEFT, padx=(0, 10))
        
        refresh_btn = tk.Button(
            drive_frame,
            text="Refresh",
            command=self.populate_drives,
            font=("Dubai", 10)
        )
        refresh_btn.pack(side=tk.LEFT)
        
        # Step 2: Output file
        step2_label = tk.Label(
            main_frame,
            text="Choose Output Location",
            font=("Dubai", 12, "bold")
        )
        step2_label.pack(anchor=tk.W, pady=(10, 5))
        
        output_frame = tk.Frame(main_frame)
        output_frame.pack(fill=tk.X, pady=(0, 20))
        
        self.output_var = tk.StringVar(value="tigr_data.csv")
        output_entry = tk.Entry(
            output_frame,
            textvariable=self.output_var,
            font=("Dubai", 10),
            width=35
        )
        output_entry.pack(side=tk.LEFT, padx=(0, 10))
        
        browse_btn = tk.Button(
            output_frame,
            text="Browse",
            command=self.browse_output,
            font=("Dubai", 10)
        )
        browse_btn.pack(side=tk.LEFT)
        
        # Step 3: Extract
        step3_label = tk.Label(
            main_frame,
            text="Extract Raw Bits",
            font=("Dubai", 12, "bold")
        )
        step3_label.pack(anchor=tk.W, pady=(10, 5))
        
        button_frame = tk.Frame(main_frame)
        button_frame.pack(pady=20)
        
        self.extract_btn = tk.Button(
            button_frame,
            text="Extract Muon Data",
            command=self.extract_data,
            font=("Dubai", 14, "bold"),
            bg="#3b82f6",
            fg="white",
            padx=30,
            pady=15,
            relief=tk.RAISED,
            cursor="hand2"
        )
        self.extract_btn.pack()
        
        # Status
        self.status_label = tk.Label(
            main_frame,
            text="Ready to extract",
            font=("Dubai", 10),
            fg="#6b7280"
        )
        self.status_label.pack(pady=10)
    
    def populate_drives(self):
        """Populate the drive selection dropdown"""
        try:
            # Get list of physical drives using wmic with CSV format
            result = subprocess.run(
                ['wmic', 'diskdrive', 'get', 'deviceid,size,caption', '/format:csv'],
                capture_output=True,
                text=True
            )
            
            self.drive_map = {}  # Store device ID mapping
            drives = []
            lines = result.stdout.strip().split('\n')
            
            for line in lines[1:]:  # Skip header
                if line.strip() and ',' in line:
                    parts = [p.strip() for p in line.split(',')]
                    # CSV format: Node,Caption,DeviceID,Size
                    if len(parts) >= 4 and 'PHYSICALDRIVE' in parts[2].upper():
                        caption = parts[1]
                        device_id = parts[2]  # This is the actual device path
                        try:
                            size_bytes = int(parts[3]) if parts[3] else 0
                            size_gb = size_bytes / (1024**3)
                            size_mb = size_bytes / (1024**2)
                            
                            # Format display string
                            if size_gb >= 1:
                                size_str = f"{size_gb:.1f} GB"
                            else:
                                size_str = f"{size_mb:.0f} MB"
                            
                            display_text = f"{device_id} - {size_str} - {caption}"
                            drives.append(display_text)
                            
                            # Store mapping
                            self.drive_map[display_text] = device_id
                        except:
                            display_text = f"{device_id} - {caption}"
                            drives.append(display_text)
                            self.drive_map[display_text] = device_id
            
            self.drive_combo['values'] = drives
            if drives:
                self.drive_combo.current(len(drives) - 1)  # Select last drive (usually SD card)
            
        except Exception as e:
            messagebox.showerror("Error", f"Could not list drives: {e}")
    
    def browse_output(self):
        """Browse for output file location"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
            initialfile="tigr_data.csv"
        )
        if filename:
            self.output_var.set(filename)
    
    def request_admin(self):
        """Relaunch with admin privileges"""
        try:
            ctypes.windll.shell32.ShellExecuteW(
                None, "runas", sys.executable, f'"{__file__}"', None, 1
            )
            self.root.quit()
        except:
            messagebox.showerror(
                "Error",
                "Failed to request administrator privileges"
            )
    
    def extract_data(self):
        """Extract data from SD card"""
        if not self.is_admin:
            messagebox.showwarning(
                "Admin Required",
                "Administrator privileges are required to read raw disk data.\n\n"
                "Please click 'Request Admin' button first."
            )
            return
        
        drive_selection = self.drive_var.get()
        if not drive_selection:
            messagebox.showwarning("No Drive", "Please select a drive first")
            return
        
        # Get actual device ID from mapping
        device_id = self.drive_map.get(drive_selection)
        if not device_id:
            messagebox.showerror("Error", "Could not determine device path")
            return
        
        output_file = self.output_var.get()
        
        # Confirm
        result = messagebox.askyesno(
            "Confirm Extraction",
            f"Extract data from:\n{drive_selection}\n\n"
            f"Device: {device_id}\n\n"
            f"Output to:\n{output_file}\n\n"
            "Continue?"
        )
        
        if not result:
            return
        
        self.status_label.config(text="Extracting data...", fg="#f59e0b")
        self.extract_btn.config(state=tk.DISABLED)
        self.root.update()
        
        try:
            # Read device - use the actual device ID path
            device_path = device_id
            
            print(f"Opening device: {device_path}")  # Debug
            
            with open(device_path, 'rb') as device:
                data = device.read(512 * 1000)  # Read 1000 sectors
            
            print(f"Read {len(data)} bytes")  # Debug
            
            # Convert to text
            text = data.decode('ascii', errors='ignore').replace('\x00', '')
            
            # Find CSV data
            if "Muon#,Band" not in text:
                raise ValueError("No TIGR data found on this device")
            
            # Extract CSV
            start_idx = text.find("Muon#,Band")
            csv_data = text[start_idx:]
            
            # Parse valid lines
            lines = csv_data.split('\n')
            valid_lines = []
            
            for line in lines:
                if ',' in line and line.strip():
                    parts = line.split(',')
                    if len(parts) >= 5 or 'Muon#' in line:
                        valid_lines.append(line)
            
            # Write to file
            with open(output_file, 'w') as f:
                f.write('\n'.join(valid_lines))
            
            count = len(valid_lines) - 1  # Exclude header
            
            self.status_label.config(
                text=f"✅ Success! Extracted {count} readings",
                fg="#059669"
            )
            
            # Show success message
            messagebox.showinfo(
                "Success!",
                f"Successfully extracted {count} readings!\n\n"
                f"Saved to: {output_file}\n\n"
                "Opening TIGR Analyzer..."
            )
            
            # Auto-open analyzer
            self.open_analyzer(output_file)
            
        except PermissionError:
            self.status_label.config(text="❌ Permission denied", fg="#dc2626")
            messagebox.showerror(
                "Permission Denied",
                "Could not read from device. Make sure you're running as Administrator."
            )
        except FileNotFoundError as e:
            self.status_label.config(text="❌ Device not found", fg="#dc2626")
            messagebox.showerror(
                "Device Not Found",
                f"Could not open device:\n{device_path}\n\n"
                f"Error: {str(e)}\n\n"
                "Try refreshing the drive list."
            )
        except Exception as e:
            self.status_label.config(text="❌ Extraction failed", fg="#dc2626")
            messagebox.showerror("Error", f"Extraction failed:\n{str(e)}")
        finally:
            self.extract_btn.config(state=tk.NORMAL)
    
    def open_analyzer(self, csv_file):
        """Open the web analyzer"""
        # Try auto-loading version first, fall back to regular
        analyzer_files = ["tigr_analyzer_autoload.html", "tigr_analyzer.html"]
        
        for analyzer_path in analyzer_files:
            if os.path.exists(analyzer_path):
                abs_path = os.path.abspath(analyzer_path)
                webbrowser.open(f'file:///{abs_path}')
                
                if 'autoload' in analyzer_path:
                    print(f"✓ Opened auto-loading analyzer")
                    # File will auto-load if tigr_data.csv is in same directory
                else:
                    print(f"✓ Opened standard analyzer")
                    messagebox.showinfo(
                        "Upload File",
                        f"Please upload the extracted data file:\n{csv_file}"
                    )
                return
        
        # No analyzer found
        messagebox.showwarning(
            "Analyzer Not Found",
            f"Could not find tigr_analyzer.html\n\n"
            f"Please open it manually and upload:\n{csv_file}"
        )

def main():
    root = tk.Tk()
    app = TIGRExtractorGUI(root)
    root.mainloop()

if __name__ == '__main__':
    main()