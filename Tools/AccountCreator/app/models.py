from dataclasses import dataclass
from typing import Optional


@dataclass
class DBAdminConfig:
    """DB Admin connection settings (for authorized personnel)"""
    host: str = ""
    port: int = 3306
    admin_id: str = ""
    admin_password: str = ""
    database: str = "game_db"  # Default database name

    def is_valid(self) -> bool:
        return all([self.host, self.admin_id, self.admin_password])


@dataclass
class AccountInfo:
    """Account information to be created"""
    account_id: str = ""
    password: str = ""

    def is_valid(self) -> bool:
        return bool(self.account_id and self.password)


@dataclass
class Preset:
    """Saved preset"""
    name: str
    config: DBAdminConfig