# Account Creator Tool

Account creation tool for DarkEden / DK Phoenix Reborn.

## Features
- **DB Admin connection settings**: Host/IP, Port, Admin ID, Admin Password, Database
- **Preset save/load/delete**: Stored as JSON in `presets/` folder
- **Connection test**: Verify MySQL connectivity
- **Account creation**: Account ID + Password → INSERT into `Player` table using MySQL `PASSWORD()`

## Installation
```bash
cd Tools/AccountCreator
run_account_creator.bat
```
Or manually:
```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python -m app.main
```

## Database Schema
Expected `Player` table:
```sql
CREATE TABLE Player (
    PlayerID VARCHAR(50) PRIMARY KEY,
    Password VARCHAR(255) NOT NULL
);
```

> **Note**: The tool uses `INSERT INTO Player (PlayerID, Password) VALUES (%s, PASSWORD(%s))` with MySQL's `PASSWORD()` function for hashing.

## Preset Files
Stored as JSON in `presets/`:
```json
{
  "name": "production",
  "host": "192.168.1.100",
  "port": 3306,
  "admin_id": "root",
  "admin_password": "password123",
  "database": "game_db"
}
```

## GUI
- **DB Admin Connection**: Required info for authorized DB access
- **Preset Save/Load/Delete**: Avoid re-entering connection settings
- **Test Connection**: Verify MySQL connectivity
- **Account to Create**: Account ID + Password (player account)
- **Create Account**: Insert account into database