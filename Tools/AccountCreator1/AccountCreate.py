import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import json
import os
import pymysql

class AccountCreatorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("MySQL Player Account Creator")
        self.root.geometry("480x620")
        self.root.resizable(False, False)
        
        # Variables for DB Connection
        self.db_host = tk.StringVar()
        self.db_port = tk.StringVar(value="3306")
        self.db_user = tk.StringVar()
        self.db_pass = tk.StringVar()
        
        # Variable for MySQL Version selection
        self.mysql_version = tk.StringVar(value="5.6")
        
        # Variables for Account Creation
        self.acc_id = tk.StringVar()
        self.acc_pass = tk.StringVar()
        self.allow_access = tk.BooleanVar(value=True) # Default to ALLOW (Checked)

        self.create_widgets()

    def create_widgets(self):
        # --- Preset Section ---
        preset_frame = ttk.LabelFrame(self.root, text="Preset (DB Connection Settings)", padding=10)
        preset_frame.pack(fill="x", padx=10, pady=5)

        ttk.Button(preset_frame, text="Save Settings (JSON)", command=self.save_preset).pack(side="left", padx=5)
        ttk.Button(preset_frame, text="Load Settings (JSON)", command=self.load_preset).pack(side="left", padx=5)

        # --- DB Admin Connection Section ---
        db_frame = ttk.LabelFrame(self.root, text="DB Admin Connection", padding=10)
        db_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(db_frame, text="DB IP/Host:").grid(row=0, column=0, sticky="w", pady=2)
        ttk.Entry(db_frame, textvariable=self.db_host, width=30).grid(row=0, column=1, pady=2)

        ttk.Label(db_frame, text="DB Port:").grid(row=1, column=0, sticky="w", pady=2)
        ttk.Entry(db_frame, textvariable=self.db_port, width=30).grid(row=1, column=1, pady=2)

        ttk.Label(db_frame, text="DB Admin ID:").grid(row=2, column=0, sticky="w", pady=2)
        ttk.Entry(db_frame, textvariable=self.db_user, width=30).grid(row=2, column=1, pady=2)

        ttk.Label(db_frame, text="DB Admin Pass:").grid(row=3, column=0, sticky="w", pady=2)
        ttk.Entry(db_frame, textvariable=self.db_pass, width=30, show="*").grid(row=3, column=1, pady=2)

        # --- MySQL Version Selection ---
        ver_frame = ttk.LabelFrame(self.root, text="MySQL Version", padding=10)
        ver_frame.pack(fill="x", padx=10, pady=5)

        ttk.Radiobutton(ver_frame, text="MySQL 5.6", 
                        variable=self.mysql_version, value="5.6").pack(anchor="w")
        ttk.Radiobutton(ver_frame, text="MySQL Latest / 8.0+", 
                        variable=self.mysql_version, value="latest").pack(anchor="w")

        # --- Account Creation Section ---
        acc_frame = ttk.LabelFrame(self.root, text="New Account Details", padding=10)
        acc_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(acc_frame, text="Account ID:").grid(row=0, column=0, sticky="w", pady=2)
        ttk.Entry(acc_frame, textvariable=self.acc_id, width=30).grid(row=0, column=1, pady=2)

        ttk.Label(acc_frame, text="Account Password:").grid(row=1, column=0, sticky="w", pady=2)
        ttk.Entry(acc_frame, textvariable=self.acc_pass, width=30, show="*").grid(row=1, column=1, pady=2)

        # Checkbox for ALLOW/DENY
        ttk.Checkbutton(acc_frame, text="Access Permission (ALLOW)", variable=self.allow_access).grid(row=2, column=1, sticky="w", pady=5)

        ttk.Button(acc_frame, text="[ Create Account ]", command=self.create_account).grid(row=3, column=0, columnspan=2, pady=10)

        # --- Log / Status Section ---
        log_frame = ttk.LabelFrame(self.root, text="Operation Log", padding=10)
        log_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.log_text = tk.Text(log_frame, height=8, state="disabled", bg="#f4f4f4")
        self.log_text.pack(fill="both", expand=True)

    def log_message(self, msg):
        """Appends a message to the log text box."""
        self.log_text.config(state="normal")
        self.log_text.insert(tk.END, msg + "\n")
        self.log_text.see(tk.END)
        self.log_text.config(state="disabled")

    def save_preset(self):
        """Saves DB connection settings to a JSON file."""
        data = {
            "host": self.db_host.get(),
            "port": self.db_port.get(),
            "user": self.db_user.get(),
            "password": self.db_pass.get() 
        }
        file_path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON files", "*.json")])
        if file_path:
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=4)
            self.log_message(f"[SUCCESS] Preset saved to: {file_path}")

    def load_preset(self):
        """Loads DB connection settings from a JSON file."""
        file_path = filedialog.askopenfilename(filetypes=[("JSON files", "*.json")])
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                self.db_host.set(data.get("host", ""))
                self.db_port.set(data.get("port", "3306"))
                self.db_user.set(data.get("user", ""))
                self.db_pass.set(data.get("password", ""))
                self.log_message(f"[SUCCESS] Preset loaded from: {file_path}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load preset:\n{e}")

    def create_account(self):
        """Executes the SQL INSERT command based on the selected MySQL version."""
        # Validation checks
        if not all([self.db_host.get(), self.db_port.get(), self.db_user.get(), self.db_pass.get()]):
            messagebox.showwarning("Missing Information", "Please fill in all DB Connection details.")
            return
            
        if not self.acc_id.get() or not self.acc_pass.get():
            messagebox.showwarning("Missing Information", "Please fill in the Account ID and Password fields.")
            return

        # Determine Access value based on Checkbox
        access_val = 'ALLOW' if self.allow_access.get() else 'DENY'
        
        # Determine SQL query based on MySQL Version
        if self.mysql_version.get() == "5.6":
            # MySQL 5.6 uses the native PASSWORD() function
            query = "INSERT INTO Player (PlayerID, Password, Access) VALUES (%s, PASSWORD(%s), %s)"
        else:
            # MySQL 8.0+ (Latest) removed PASSWORD(), so we use SHA2 256-bit hash
            query = "INSERT INTO Player (PlayerID, Password, Access) VALUES (%s, SHA2(%s, 256), %s)"

        params = (self.acc_id.get(), self.acc_pass.get(), access_val)
        connection = None

        try:
            self.log_message(f"Connecting to {self.db_host.get()}:{self.db_port.get()}...")
            
            # Establish database connection
            connection = pymysql.connect(
                host=self.db_host.get(),
                port=int(self.db_port.get()),
                user=self.db_user.get(),
                password=self.db_pass.get(),
                charset='utf8mb4',
                cursorclass=pymysql.cursors.DictCursor
            )

            # Execute query
            with connection.cursor() as cursor:
                cursor.execute(query, params)
            connection.commit()
            
            self.log_message(f"[SUCCESS] Account '{self.acc_id.get()}' created. (Access: {access_val})")
            messagebox.showinfo("Success", f"Account created successfully!\nID: {self.acc_id.get()}\nAccess: {access_val}")
            
            # Clear account fields for the next entry
            self.acc_id.set("")
            self.acc_pass.set("")

        except pymysql.MySQLError as e:
            self.log_message(f"[ERROR] MySQL Error: {e}")
            messagebox.showerror("MySQL Error", f"Database operation failed:\n{e}")
        except Exception as e:
            self.log_message(f"[ERROR] Connection Error: {e}")
            messagebox.showerror("Connection Error", f"Could not connect to the database:\n{e}")
        finally:
            # Ensure connection is closed properly
            if connection and connection.open:
                connection.close()

if __name__ == "__main__":
    root = tk.Tk()
    app = AccountCreatorApp(root)
    root.mainloop()
