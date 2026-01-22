#!/usr/bin/env python3
"""
Test script updated to match Database.java logic and column names
"""

import socket
import time
import threading
import os
import sys

# הגדרות שרת
SERVER_HOST = "127.0.0.1"
SERVER_PORT = 7778

def send_sql(sql: str) -> str:
    """שולח פקודת SQL לשרת ומחזיר את התשובה (מטפל ב-Null Terminator)"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_HOST, SERVER_PORT))
        
        # שליחת הפקודה עם Null Terminator (כפי שה-Java עושה)
        sock.sendall(sql.encode('utf-8') + b'\0')
        
        # קבלת התשובה
        data = b""
        while True:
            chunk = sock.recv(1024)
            if not chunk:
                break
            data += chunk
            if b"\0" in data:
                msg, _ = data.split(b"\0", 1)
                sock.close()
                return msg.decode('utf-8')
        
        sock.close()
        return ""
    except Exception as e:
        return f"Connection Error: {e}"

def run_tests():
    print("=" * 80)
    print("Integration Test: Python SQL Server vs Database.java Logic")
    print("=" * 80)
    
    time.sleep(1) # המתנה לשרת שיעלה
    
    # טסט 1: הוספת משתמש (תואם ל-login ב-Java)
    print("\n[TEST 1] Inserting user (Matches Database.java login logic)...")
    sql_insert_user = "INSERT INTO users (username, password, registration_date) VALUES ('alice', 'pass123', datetime('now'))"
    response = send_sql(sql_insert_user)
    print(f"Response: {response}")
    assert response.startswith("SUCCESS"), f"Expected SUCCESS, got '{response}'"
    print("✓ PASS")
    
    # טסט 2: בדיקת סיסמה (תואם ל-isExist ב-Java)
    print("\n[TEST 2] Querying password (Matches Database.java check logic)...")
    sql_check_pass = "SELECT password FROM users WHERE username='alice'"
    response = send_sql(sql_check_pass)
    print(f"Response: {response}")
    assert "pass123" in response, f"Missing password in response: '{response}'"
    print("✓ PASS")
    
    # טסט 3: תיעוד כניסה (תואם ל-logLogin ב-Java)
    print("\n[TEST 3] Recording login (Matches Database.java logLogin)...")
    sql_log_login = "INSERT INTO login_history (username, login_time) VALUES ('alice', datetime('now'))"
    response = send_sql(sql_log_login)
    print(f"Response: {response}")
    assert response.startswith("SUCCESS"), "Failed to log login"
    print("✓ PASS")
    
    # טסט 4: תיעוד העלאת קובץ (תואם ל-trackFileUpload ב-Java)
    # שים לב לשמות העמודות: filename, upload_time, game_channel
    print("\n[TEST 4] Tracking file upload (Matches Database.java trackFileUpload)...")
    sql_track_file = "INSERT INTO file_tracking (username, filename, upload_time, game_channel) VALUES ('alice', 'events.json', datetime('now'), 'germany_israel')"
    response = send_sql(sql_track_file)
    print(f"Response: {response}")
    assert response.startswith("SUCCESS"), "Failed to track file"
    print("✓ PASS")
    
    # טסט 5: תיעוד יציאה (תואם ל-logout ב-Java)
    print("\n[TEST 5] Recording logout (Matches Database.java logout)...")
    sql_logout = "UPDATE login_history SET logout_time=datetime('now') WHERE username='alice' AND logout_time IS NULL"
    response = send_sql(sql_logout)
    print(f"Response: {response}")
    assert response.startswith("SUCCESS"), "Failed to log logout"
    print("✓ PASS")
    
    # טסט 6: הרצת הדו"ח המיוחד של השרת
    print("\n[TEST 6] Triggering SERVER REPORT...")
    response = send_sql("REPORT")
    print(f"Response: {response}")
    assert "Report printed" in response, "REPORT command failed"
    print("✓ PASS")

    # טסט 7: בדיקת שליפת נתונים לדו"ח ב-Java (תואם ל-printReport)
    print("\n[TEST 7] Querying all file uploads (Matches Database.java printReport query)...")
    sql_get_files = "SELECT username, filename, upload_time, game_channel FROM file_tracking ORDER BY username, upload_time DESC"
    response = send_sql(sql_get_files)
    print(f"Response: {response}")
    assert "events.json" in response and "germany_israel" in response, "Missing record data"
    print("✓ PASS")

    print("\n" + "=" * 80)
    print("ALL TESTS PASSED! Your Java and Python SQL sync is perfect. ✓")
    print("=" * 80)

if __name__ == "__main__":
    # מניח שקובץ השרת נמצא באותה תיקייה
    try:
        from sql_server import start_server
    except ImportError:
        print("Error: Could not find sql_server.py. Make sure it is in the same directory.")
        sys.exit(1)
        
    # ניקוי דאטה בייס ישן
    if os.path.exists("stomp_server.db"):
        os.remove("stomp_server.db")
    
    # הרצת השרת בת'רד נפרד
    server_thread = threading.Thread(target=lambda: start_server(port=SERVER_PORT), daemon=True)
    server_thread.start()
    
    try:
        run_tests()
    except AssertionError as e:
        print(f"\n✗ TEST FAILED: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted")