import mysql.connector
from mysql.connector import Error
from typing import Optional, Tuple

from app.models import DBAdminConfig, AccountInfo


class DatabaseError(Exception):
    """Database error"""
    pass


def create_connection(config: DBAdminConfig) -> mysql.connector.MySQLConnection:
    """Create MySQL connection"""
    try:
        conn = mysql.connector.connect(
            host=config.host,
            port=config.port,
            user=config.admin_id,
            password=config.admin_password,
            database=config.database,
            autocommit=True,
        )
        return conn
    except Error as e:
        raise DatabaseError(f"DB connection error: {e}")


def test_connection(config: DBAdminConfig) -> Tuple[bool, str]:
    """Test connection"""
    try:
        conn = create_connection(config)
        conn.close()
        return True, "Connection successful"
    except DatabaseError as e:
        return False, str(e)


def create_account(config: DBAdminConfig, account: AccountInfo) -> Tuple[bool, str]:
    """
    Create account: INSERT INTO Player (PlayerID, Password) VALUES (%s, PASSWORD(%s))
    Uses MySQL PASSWORD() function.
    """
    sql = "INSERT INTO Player (PlayerID, Password) VALUES (%s, PASSWORD(%s))"
    try:
        conn = create_connection(config)
        cursor = conn.cursor()
        cursor.execute(sql, (account.account_id, account.password))
        conn.commit()
        cursor.close()
        conn.close()
        return True, f"Account created successfully: {account.account_id}"
    except Error as e:
        if e.errno == 1062:  # Duplicate entry
            return False, f"Account '{account.account_id}' already exists"
        raise DatabaseError(f"Account creation error: {e}")


def check_account_exists(config: DBAdminConfig, account_id: str) -> Tuple[bool, Optional[str]]:
    """Check if account exists"""
    sql = "SELECT PlayerID FROM Player WHERE PlayerID = %s LIMIT 1"
    try:
        conn = create_connection(config)
        cursor = conn.cursor()
        cursor.execute(sql, (account_id,))
        row = cursor.fetchone()
        cursor.close()
        conn.close()
        return row is not None, row[0] if row else None
    except Error as e:
        raise DatabaseError(f"Check error: {e}")