import json
from pathlib import Path
from typing import List, Optional

from app.models import DBAdminConfig, Preset


PRESETS_DIR = Path(__file__).parent.parent / "presets"
PRESETS_DIR.mkdir(exist_ok=True)


def get_preset_path(name: str) -> Path:
    safe_name = "".join(c for c in name if c.isalnum() or c in ("-", "_")).strip()
    return PRESETS_DIR / f"{safe_name}.json"


def save_preset(name: str, config: DBAdminConfig) -> Path:
    """Save preset as JSON"""
    path = get_preset_path(name)
    data = {
        "name": name,
        "host": config.host,
        "port": config.port,
        "admin_id": config.admin_id,
        "admin_password": config.admin_password,
        "database": config.database,
    }
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    return path


def load_preset(name: str) -> Optional[DBAdminConfig]:
    """Load preset from JSON"""
    path = get_preset_path(name)
    if not path.exists():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        return DBAdminConfig(
            host=data.get("host", ""),
            port=data.get("port", 3306),
            admin_id=data.get("admin_id", ""),
            admin_password=data.get("admin_password", ""),
            database=data.get("database", "game_db"),
        )
    except Exception:
        return None


def list_presets() -> List[str]:
    """List saved preset names"""
    return [p.stem for p in PRESETS_DIR.glob("*.json")]


def delete_preset(name: str) -> bool:
    """Delete preset"""
    path = get_preset_path(name)
    if path.exists():
        path.unlink()
        return True
    return False