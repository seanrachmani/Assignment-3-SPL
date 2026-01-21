import subprocess
import time
import os
import signal
import json

# --- הגדרות ---
SERVER_PORT = "7777"
SERVER_CMD = ["mvn", "exec:java", "-Dexec.mainClass=bgu.spl.net.impl.stomp.StompServer", f"-Dexec.args={SERVER_PORT} reactor"]
CLIENT_PATH = "./bin/StompWCIClient"
JSON_FILE = "events_test.json"
SUMMARY_FILE = "summary_test.txt"

# --- יצירת קובץ אירועים לדוגמה ---
def create_dummy_json():
    data = {
        "team a": "Germany",
        "team b": "Japan",
        "events": [
            {
                "event name": "Kickoff",
                "time": 0,
                "general game updates": {"active": "true"},
                "team a updates": {},
                "team b updates": {},
                "description": "The game has started!"
            },
            {
                "event name": "Goal",
                "time": 15,
                "general game updates": {"active": "true", "score": "1-0"},
                "team a updates": {"goals": "1"},
                "team b updates": {},
                "description": "Germany scores!"
            }
        ]
    }
    with open(JSON_FILE, 'w') as f:
        json.dump(data, f)
    print(f"[TEST] Created dummy file: {JSON_FILE}")

# --- פונקציה שמנקה קבצים ישנים ---
def cleanup_files():
    for f in [JSON_FILE, SUMMARY_FILE]:
        if os.path.exists(f):
            os.remove(f)

# --- הרצת הבדיקה ---
def run_test():
    cleanup_files()
    create_dummy_json()

    print(f"[TEST] Starting Server on port {SERVER_PORT}...")
    # הפעלת השרת ברקע
    server_process = subprocess.Popen(SERVER_CMD, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    # נותנים לשרת זמן לעלות
    time.sleep(3) 

    try:
        print("[TEST] Starting User B (The Subscriber)...")
        # פתיחת קליינט ב'
        user_b = subprocess.Popen([CLIENT_PATH], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        print("[TEST] Starting User A (The Reporter)...")
        # פתיחת קליינט א'
        user_a = subprocess.Popen([CLIENT_PATH], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        # --- תסריט הפקודות ---
        
        # 1. התחברות של שניהם
        print("[TEST] Logging in both users...")
        user_b.stdin.write(f"login 127.0.0.1:{SERVER_PORT} userB passB\n")
        user_b.stdin.flush()
        
        user_a.stdin.write(f"login 127.0.0.1:{SERVER_PORT} userA passA\n")
        user_a.stdin.flush()
        time.sleep(1) # חיכוי קצר לוודא חיבור

        # 2. הצטרפות לערוץ
        channel_name = "Germany_Japan"
        print(f"[TEST] Joining channel {channel_name}...")
        user_b.stdin.write(f"join {channel_name}\n")
        user_b.stdin.flush()

        user_a.stdin.write(f"join {channel_name}\n")
        user_a.stdin.flush()
        time.sleep(1)

        # 3. דיווח אירועים ע"י User A
        print(f"[TEST] User A is reporting events from {JSON_FILE}...")
        user_a.stdin.write(f"report {JSON_FILE}\n")
        user_a.stdin.flush()

        # נותנים זמן להודעות להגיע מהשרת ל-User B
        time.sleep(2)

        # 4. יצירת סיכום ע"י User B
        print("[TEST] User B is generating summary...")
        # הפקודה: summary [game_name] [user_to_filter] [output_file]
        user_b.stdin.write(f"summary {channel_name} userA {SUMMARY_FILE}\n")
        user_b.stdin.flush()
        time.sleep(1)

        # 5. בדיקת קובץ הסיכום
        if os.path.exists(SUMMARY_FILE):
            with open(SUMMARY_FILE, 'r') as f:
                content = f.read()
                print("\n--- Summary Content Start ---")
                print(content)
                print("--- Summary Content End ---\n")
                
                # בדיקות ולידציה (Asserts)
                if "Germany vs Japan" in content and "Germany scores!" in content:
                    print("\033[92m[SUCCESS] Test Passed! Summary contains expected data.\033[0m")
                else:
                    print("\033[91m[FAILURE] Summary is missing expected data.\033[0m")
        else:
            print("\033[91m[FAILURE] Summary file was not created!\033[0m")

        # 6. יציאה מסודרת
        print("[TEST] Logging out...")
        user_a.stdin.write("logout\n")
        user_b.stdin.write("logout\n")
        time.sleep(1)

    except Exception as e:
        print(f"\033[91m[ERROR] An error occurred: {e}\033[0m")
    
    finally:
        # הרג תהליכים בסוף הריצה
        print("[TEST] Cleaning up processes...")
        user_a.kill()
        user_b.kill()
        # השרת רץ כ-subprocess של maven ולפעמים קשה להרוג אותו ישירות, מנסים בכל זאת
        server_process.terminate()
        # פקודת מערכת לוודא שאין תהליכים תלויים (Linux/Mac)
        os.system(f"lsof -ti:{SERVER_PORT} | xargs kill -9 2>/dev/null")
        cleanup_files()
        print("[TEST] Done.")

if __name__ == "__main__":
    run_test()