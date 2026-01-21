#!/usr/bin/env python3
import socket
import time
import threading
import os
import sys

def send_sql(sql: str, port=7778) -> str:
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("127.0.0.1", port))
        sock.sendall(sql.encode('utf-8') + b'\0')
        data = b""
        while True:
            chunk = sock.recv(1024)
            if not chunk: break
            data += chunk
            if b"\0" in data:
                msg, _ = data.split(b"\0", 1)
                sock.close()
                return msg.decode('utf-8')
        return ""
    except Exception as e:
        return f"Connection Error: {e}"

def run_tests():
    print("=" * 80)
    print("SQL Server Test Suite - SYNCED WITH YOUR NAMES")
    print("=" * 80)
    time.sleep(1)
    
    # TEST 1
    print("\n[TEST 1] Inserting user...")
    response = send_sql("INSERT INTO users (username, password, registration_date) VALUES ('alice', 'pass123', datetime('now'))")
    assert response.startswith("SUCCESS"), f"Failed: {response}"
    print("✓ PASS")
    
    # TEST 3
    print("\n[TEST 3] Querying users...")
    response = send_sql("SELECT username FROM users WHERE username='alice'")
    assert "alice" in response
    print("✓ PASS")

    # TEST 6
    print("\n[TEST 6] Recording logout...")
    send_sql("INSERT INTO login_history (username, login_time) VALUES ('alice', datetime('now'))")
    response = send_sql("UPDATE login_history SET logout_time=datetime('now') WHERE username='alice' AND logout_time IS NULL")
    assert response.startswith("SUCCESS")
    print("✓ PASS")

    # TEST 7 - מותאם לשמות שלך
    print("\n[TEST 7] Tracking file upload (Using your column names)...")
    sql_7 = "INSERT INTO file_tracking (file_name, username_of_submitter, game_channel, date_time) VALUES ('events.json', 'alice', 'game1', datetime('now'))"
    response = send_sql(sql_7)
    assert response.startswith("SUCCESS")
    print("✓ PASS")
    
    # TEST 8 - מותאם לשמות שלך
    print("\n[TEST 8] Querying file uploads...")
    response = send_sql("SELECT file_name FROM file_tracking WHERE username_of_submitter='alice'")
    assert "events.json" in response
    print("✓ PASS")
    
    print("\n" + "=" * 80)
    print("ALL TESTS PASSED! ✓")
    print("=" * 80)

if __name__ == "__main__":
    if os.path.exists("stomp_server.db"):
        try: os.remove("stomp_server.db")
        except: pass

    try:
        from sql_server import start_server 
    except ImportError:
        print("Error: Name the server file 'sql_server.py'")
        sys.exit(1)

    threading.Thread(target=lambda: start_server(port=7778), daemon=True).start()
    run_tests()