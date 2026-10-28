import sys
from pathlib import Path

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QApplication,
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QInputDialog,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from app.db import create_account, test_connection, DatabaseError
from app.models import DBAdminConfig, AccountInfo
from app.presets import save_preset, load_preset, list_presets, delete_preset


class PresetDialog(QDialog):
    """Preset save/load/delete dialog"""
    def __init__(self, parent=None, mode: str = "save"):
        super().__init__(parent)
        self.mode = mode  # "save", "load", "delete"
        self.setWindowTitle({"save": "Save Preset", "load": "Load Preset", "delete": "Delete Preset"}[mode])
        self.resize(350, 150)

        layout = QVBoxLayout(self)

        if mode == "save":
            self.name_edit = QLineEdit()
            self.name_edit.setPlaceholderText("Preset name (e.g. production, test, local)")
            layout.addWidget(self.name_edit)
        else:
            self.combo = QComboBox()
            self.combo.addItems(list_presets())
            layout.addWidget(self.combo)

        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def get_value(self) -> str:
        if self.mode == "save":
            return self.name_edit.text().strip()
        return self.combo.currentText()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Account Creator")
        self.resize(500, 550)

        self.db_config = DBAdminConfig()
        self._setup_ui()
        self._load_last_preset()

    def _setup_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(12)
        main_layout.setContentsMargins(16, 16, 16, 16)

        # --- DB Admin Connection Group ---
        db_group = QGroupBox("DB Admin Connection (Authorized Personnel)")
        db_layout = QFormLayout(db_group)
        db_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        db_layout.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.ExpandingFieldsGrow)
        db_layout.setSpacing(10)

        self.host_edit = QLineEdit()
        self.host_edit.setPlaceholderText("127.0.0.1 or db.example.com")
        db_layout.addRow("DB IP/Host:", self.host_edit)

        self.port_edit = QLineEdit("3306")
        self.port_edit.setPlaceholderText("3306")
        db_layout.addRow("DB Port:", self.port_edit)

        self.admin_id_edit = QLineEdit()
        self.admin_id_edit.setPlaceholderText("root or admin user")
        db_layout.addRow("DB Admin ID:", self.admin_id_edit)

        self.admin_pass_edit = QLineEdit()
        self.admin_pass_edit.setEchoMode(QLineEdit.EchoMode.Password)
        self.admin_pass_edit.setPlaceholderText("Admin password")
        db_layout.addRow("DB Admin Password:", self.admin_pass_edit)

        self.db_name_edit = QLineEdit("game_db")
        self.db_name_edit.setPlaceholderText("Database name")
        db_layout.addRow("Database:", self.db_name_edit)

        # Preset buttons
        preset_layout = QHBoxLayout()
        self.save_preset_btn = QPushButton("Save Preset")
        self.load_preset_btn = QPushButton("Load Preset")
        self.delete_preset_btn = QPushButton("Delete Preset")
        preset_layout.addWidget(self.save_preset_btn)
        preset_layout.addWidget(self.load_preset_btn)
        preset_layout.addWidget(self.delete_preset_btn)
        db_layout.addRow("", preset_layout)

        self.test_conn_btn = QPushButton("Test Connection")
        self.test_conn_btn.setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }")
        db_layout.addRow("", self.test_conn_btn)

        main_layout.addWidget(db_group)

        # --- Account Creation Group ---
        acc_group = QGroupBox("Account Information to Create")
        acc_layout = QFormLayout(acc_group)
        acc_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        acc_layout.setSpacing(10)

        self.account_id_edit = QLineEdit()
        self.account_id_edit.setPlaceholderText("Player account ID")
        acc_layout.addRow("Account ID:", self.account_id_edit)

        self.account_pass_edit = QLineEdit()
        self.account_pass_edit.setEchoMode(QLineEdit.EchoMode.Password)
        self.account_pass_edit.setPlaceholderText("Player account password")
        acc_layout.addRow("Account Password:", self.account_pass_edit)

        main_layout.addWidget(acc_group)

        # --- Create Account Button ---
        self.create_btn = QPushButton("Create Account")
        self.create_btn.setFixedHeight(48)
        self.create_btn.setStyleSheet("""
            QPushButton {
                background-color: #4CAF50;
                color: white;
                font-size: 16px;
                font-weight: bold;
                border-radius: 6px;
            }
            QPushButton:hover { background-color: #45a049; }
            QPushButton:pressed { background-color: #3d8b40; }
            QPushButton:disabled { background-color: #cccccc; color: #888888; }
        """)
        main_layout.addWidget(self.create_btn)

        # Status bar
        self.statusBar().showMessage("Ready")

        # --- Signal connections ---
        self.save_preset_btn.clicked.connect(self._on_save_preset)
        self.load_preset_btn.clicked.connect(self._on_load_preset)
        self.delete_preset_btn.clicked.connect(self._on_delete_preset)
        self.test_conn_btn.clicked.connect(self._on_test_connection)
        self.create_btn.clicked.connect(self._on_create_account)

    def _get_db_config_from_ui(self) -> DBAdminConfig:
        try:
            port = int(self.port_edit.text().strip() or "3306")
        except ValueError:
            port = 3306
        return DBAdminConfig(
            host=self.host_edit.text().strip(),
            port=port,
            admin_id=self.admin_id_edit.text().strip(),
            admin_password=self.admin_pass_edit.text(),
            database=self.db_name_edit.text().strip() or "game_db",
        )

    def _get_account_from_ui(self) -> AccountInfo:
        return AccountInfo(
            account_id=self.account_id_edit.text().strip(),
            password=self.account_pass_edit.text(),
        )

    def _set_ui_from_config(self, config: DBAdminConfig):
        self.host_edit.setText(config.host)
        self.port_edit.setText(str(config.port))
        self.admin_id_edit.setText(config.admin_id)
        self.admin_pass_edit.setText(config.admin_password)
        self.db_name_edit.setText(config.database)

    def _on_save_preset(self):
        dialog = PresetDialog(self, mode="save")
        if dialog.exec() == QDialog.DialogCode.Accepted:
            name = dialog.get_value()
            if not name:
                QMessageBox.warning(self, "Warning", "Preset name cannot be empty")
                return
            config = self._get_db_config_from_ui()
            if not config.is_valid():
                QMessageBox.warning(self, "Warning", "DB Admin info is incomplete")
                return
            save_preset(name, config)
            self.statusBar().showMessage(f"Preset '{name}' saved")
            QMessageBox.information(self, "Success", f"Preset '{name}' saved")

    def _on_load_preset(self):
        presets = list_presets()
        if not presets:
            QMessageBox.information(self, "Info", "No saved presets")
            return
        dialog = PresetDialog(self, mode="load")
        if dialog.exec() == QDialog.DialogCode.Accepted:
            name = dialog.get_value()
            config = load_preset(name)
            if config:
                self._set_ui_from_config(config)
                self.statusBar().showMessage(f"Preset '{name}' loaded")
            else:
                QMessageBox.warning(self, "Error", "Failed to load preset")

    def _on_delete_preset(self):
        presets = list_presets()
        if not presets:
            QMessageBox.information(self, "Info", "No saved presets")
            return
        dialog = PresetDialog(self, mode="delete")
        if dialog.exec() == QDialog.DialogCode.Accepted:
            name = dialog.get_value()
            if delete_preset(name):
                self.statusBar().showMessage(f"Preset '{name}' deleted")
                QMessageBox.information(self, "Success", f"Preset '{name}' deleted")
            else:
                QMessageBox.warning(self, "Error", "Failed to delete preset")

    def _on_test_connection(self):
        config = self._get_db_config_from_ui()
        if not config.is_valid():
            QMessageBox.warning(self, "Warning", "DB Admin info incomplete (Host, Admin ID, Admin Password)")
            return

        self.test_conn_btn.setEnabled(False)
        self.test_conn_btn.setText("Testing...")
        QApplication.processEvents()

        try:
            success, msg = test_connection(config)
            if success:
                QMessageBox.information(self, "Success", msg)
                self.statusBar().showMessage(msg)
            else:
                QMessageBox.critical(self, "Connection Error", msg)
                self.statusBar().showMessage(f"Error: {msg}")
        except DatabaseError as e:
            QMessageBox.critical(self, "Error", str(e))
            self.statusBar().showMessage(f"Error: {e}")
        finally:
            self.test_conn_btn.setEnabled(True)
            self.test_conn_btn.setText("Test Connection")

    def _on_create_account(self):
        config = self._get_db_config_from_ui()
        account = self._get_account_from_ui()

        if not config.is_valid():
            QMessageBox.warning(self, "Warning", "DB Admin connection info incomplete")
            return
        if not account.is_valid():
            QMessageBox.warning(self, "Warning", "Account ID and Password cannot be empty")
            return

        # Confirmation dialog
        reply = QMessageBox.question(
            self,
            "Confirm",
            f"Account will be created:\n\n"
            f"DB: {config.host}:{config.port} ({config.database})\n"
            f"Admin: {config.admin_id}\n"
            f"Account ID: {account.account_id}\n\n"
            f"Continue?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if reply != QMessageBox.StandardButton.Yes:
            return

        self.create_btn.setEnabled(False)
        self.create_btn.setText("Creating...")
        QApplication.processEvents()

        try:
            success, msg = create_account(config, account)
            if success:
                QMessageBox.information(self, "Success", msg)
                self.statusBar().showMessage(msg)
                # Clear fields
                self.account_id_edit.clear()
                self.account_pass_edit.clear()
            else:
                QMessageBox.warning(self, "Warning", msg)
                self.statusBar().showMessage(msg)
        except DatabaseError as e:
            QMessageBox.critical(self, "Error", str(e))
            self.statusBar().showMessage(f"Error: {e}")
        finally:
            self.create_btn.setEnabled(True)
            self.create_btn.setText("Create Account")

    def _load_last_preset(self):
        """Auto-load last used preset (optional)"""
        presets = list_presets()
        if presets:
            # Load last saved (alphabetically last)
            last = sorted(presets)[-1]
            config = load_preset(last)
            if config:
                self._set_ui_from_config(config)
                self.statusBar().showMessage(f"Last preset '{last}' loaded")


def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")  # Modern look
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())