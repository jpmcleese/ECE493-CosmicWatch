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
import re  # ✅ Added for extracting physical drive name

class TIGRExtractorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("TIGR SD Card Data Extractor")
        self.root.geometry("600x500")
        self.root.resizable(False, False)
        
        # Check admin rights
        self.is_admin = self.check_admin()
        
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
        header_frame = tk.Frame(self.root, bg="#1e3a8a", height=80)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        title_label = tk.Label(
            header_frame, 
            text="🔬 TIGR Data Extractor",
            font=("Arial", 20, "bold"),
            bg="#1e3a8a",
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
            
            warning_icon = tk.Label(warning_frame, text="⚠️", bg="#fef3c7", font=("Arial", 16))
            warning_icon.pack(side=tk.LEFT, padx=10, pady=10)
            
            warning_text = tk.Label(
                warning_frame,
                text="Not running as Administrator!\nClick 'Request Admin' to elevate privileges.",
                bg="#fef3c7",
                fg="#92400e",
                font=("Arial", 10),
                justify=tk.LEFT
            )
            warning_text.pack(side=tk.LEFT, pady=10)
            
            admin_btn = tk.Button(
                warning_frame,
                text="Request Admin",
                command=self.request_admin,
                bg="#f59e0b",
                fg="white",
                font=("Arial", 10, "bold"),
                relief=tk.RAISED,
                cursor="hand2"
            )
            admin_btn.pack(side=tk.RIGHT, padx=10, pady=10)
        else:
            admin_frame = tk.Frame(main_frame, bg="#d1fae5", relief=tk.RAISED, bd=2)
            admin_frame.pack(fill=tk.X, pady=(0, 20))
            
            admin_label = tk.Label(
                admin_frame,
                text="✅ Running with Administrator privileges",
                bg="#d1fae5",
                fg="#065f46",
                font=("Arial", 10, "bold")
            )
            admin_label.pack(pady=10)
        
        # Step 1: Select Drive
        step1_label = tk.Label(
            main_frame,
            text="Step 1: Select your SD card drive",
            font=("Arial", 12, "bold")
        )
        step1_label.pack(anchor=tk.W, pady=(10, 5))
        
        drive_frame = tk.Frame(main_frame)
        drive_frame.pack(fill=tk.X, pady=(0, 20))
        
        self.drive_var = tk.StringVar()
        self.drive_combo = ttk.Combobox(
            drive_frame,
            textvariable=self.drive_var,
            font=("Arial", 10),
            state="readonly",
            width=40
        )
        self.drive_combo.pack(side=tk.LEFT, padx=(0, 10))
        
        refresh_btn = tk.Button(
            drive_frame,
            text="🔄 Refresh",
            command=self.populate_drives,
            font=("Arial", 10)
        )
        refresh_btn.pack(side=tk.LEFT)
        
        # Step 2: Output file
        step2_label = tk.Label(
            main_frame,
            text="Step 2: Choose output location",
            font=("Arial", 12, "bold")
        )
        step2_label.pack(anchor=tk.W, pady=(10, 5))
        
        output_frame = tk.Frame(main_frame)
        output_frame.pack(fill=tk.X, pady=(0, 20))
        
        self.output_var = tk.StringVar(value="tigr_data.csv")
        output_entry = tk.Entry(
            output_frame,
            textvariable=self.output_var,
            font=("Arial", 10),
            width=35
        )
        output_entry.pack(side=tk.LEFT, padx=(0, 10))
        
        browse_btn = tk.Button(
            output_frame,
            text="📁 Browse",
            command=self.browse_output,
            font=("Arial", 10)
        )
        browse_btn.pack(side=tk.LEFT)
        
        # Step 3: Extract
        step3_label = tk.Label(
            main_frame,
            text="Step 3: Extract data",
            font=("Arial", 12, "bold")
        )
        step3_label.pack(anchor=tk.W, pady=(10, 5))
        
        button_frame = tk.Frame(main_frame)
        button_frame.pack(pady=20)
        
        self.extract_btn = tk.Button(
            button_frame,
            text="🚀 Extract Data",
            command=self.extract_data,
            font=("Arial", 14, "bold"),
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
            font=("Arial", 10),
            fg="#6b7280"
        )
        self.status_label.pack(pady=10)
    
    def populate_drives(self):
        """Populate the drive selection dropdown"""
        try:
            # Get list of physical drives using wmic
            result = subprocess.run(
                ['wmic', 'diskdrive', 'get', 'deviceid,size,caption'],
                capture_output=True,
                text=True
            )
            
            drives = []
            lines = result.stdout.strip().split('\n')[1:]  # Skip header
            
            for line in lines:
                if line.strip() and 'PHYSICALDRIVE' in line.upper():
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        device_id = parts[0]
                        try:
                            size_bytes = int(parts[1]) if len(parts) > 1 else 0
                            size_gb = size_bytes / (1024**3)
                            caption = ' '.join(parts[2:]) if len(parts) > 2 else "Unknown"
                            drives.append(f"{caption} - {size_gb:.1f} GB (Card {device_id})")
                        except:
                            drives.append(device_id)
            
            self.drive_combo['values'] = drives
            if drives:
                self.drive_combo.current(0)
            
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

        # ✅ Extract proper physical drive ID using regex
        match = re.search(r'\\\\\.\\PHYSICALDRIVE\d+', drive_selection)
        if match:
            device_id = match.group(0)
        else:
            raise ValueError(f"Could not find physical drive ID in: {drive_selection}")

        output_file = self.output_var.get()
        
        # Confirm
        result = messagebox.askyesno(
            "Confirm Extraction",
            f"Extract data from:\n{device_id}\n\n"
            f"Output to:\n{output_file}\n\n"
            "Continue?"
        )
        
        if not result:
            return
        
        self.status_label.config(text="Extracting data...", fg="#f59e0b")
        self.extract_btn.config(state=tk.DISABLED)
        self.root.update()
        
        try:
            # Read device
            with open(device_id, 'rb') as device:
                data = device.read(512 * 1000)  # Read 1000 sectors
            
            # Convert to text
            text = data.decode('ascii', errors='ignore').replace('\x00', '')
            
            # Find CSV data
            if "Muon#,Band" not in text:
                raise ValueError("No TIGR data found on this device")
            
            start_idx = text.find("Muon#,Band")
            csv_data = text[start_idx:]
            
            lines = csv_data.split('\n')
            valid_lines = [
                line for line in lines
                if ',' in line and line.strip() and ('Muon#' in line or len(line.split(',')) >= 5)
            ]
            
            with open(output_file, 'w') as f:
                f.write('\n'.join(valid_lines))
            
            count = len(valid_lines) - 1
            
            self.status_label.config(
                text=f"✅ Success! Extracted {count} readings",
                fg="#059669"
            )
            
            result = messagebox.askyesno(
                "Success!",
                f"Successfully extracted {count} readings!\n\n"
                f"Saved to: {output_file}\n\n"
                "Open TIGR Analyzer now?"
            )
            
            if result:
                self.open_analyzer(output_file)
            
        except PermissionError:
            self.status_label.config(text="❌ Permission denied", fg="#dc2626")
            messagebox.showerror(
                "Permission Denied",
                "Could not read from device. Make sure you're running as Administrator."
            )
        except Exception as e:
            self.status_label.config(text="❌ Extraction failed", fg="#dc2626")
            messagebox.showerror("Error", f"Extraction failed:\n{str(e)}")
        finally:
            self.extract_btn.config(state=tk.NORMAL)
    
    def open_analyzer(self, csv_file):
        """Open the web analyzer"""
        analyzer_path = "tigr_analyzer.html"
        
        if os.path.exists(analyzer_path):
            abs_path = os.path.abspath(analyzer_path)
            webbrowser.open(f'file:///{abs_path}')
            
            messagebox.showinfo(
                "Analyzer Opened",
                f"TIGR Analyzer opened in your browser!\n\n"
                f"Upload the file:\n{csv_file}"
            )
        else:
            messagebox.showinfo(
                "Manual Upload",
                f"Open tigr_analyzer.html manually and upload:\n{csv_file}"
            )

def main():
    root = tk.Tk()
    app = TIGRExtractorGUI(root)
    root.mainloop()

if __name__ == '__main__':
    main()
