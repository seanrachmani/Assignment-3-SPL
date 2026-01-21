import subprocess
import time
import os
import signal
import json

# --- הגדרות ---
SERVER_PORT = "7777"
SERVER_CMD = ["mvn", "exec:java", "-Dexec.mainClass=bgu.spl.net.impl.stomp.StompServer", f"-Dexec.args={SERVER_PORT} reactor"]
CLIENT_PATH = "./client/bin/StompWCIClient"
# קבצים זמניים
JSON_1 = "test_events_part1.json" # מחצית ראשונה
JSON_2 = "test_events_part2.json" # מחצית שנייה
SUMMARY_LISTENER = "summary_listener.txt"
SUMMARY_ISOLATED = "summary_isolated.txt"

# צבעים ליופי
GREEN = "\033[92m"
RED = "\033[91m"
CYAN = "\033[96m"
YELLOW = "\033[93m"
RESET = "\033[0m"

def print_status(msg):
    print(f"{CYAN}[TEST]{RESET} {msg}")

def print_pass(msg):
    print(f"{GREEN}[PASS]{RESET} {msg}")

def print_fail(msg):
    print(f"{RED}[FAIL]{RESET} {msg}")

# --- יצירת קבצי JSON ---
def create_test_files():
    # קובץ 1: התחלה
    data1 = {
        "team a": "Germany", "team b": "Japan",
        "events": [
            {
                "event name": "Kickoff", "time": 0,
                "general game updates": {"active": "true"},
                "team a updates": {}, "team b updates": {},
                "description": "Game Started."
            }
        ]
    }
    # קובץ 2: גול (נשלח אחרי שאחד המשתמשים יעשה exit)
    data2 = {
        "team a": "Germany", "team b": "Japan",
        "events": [
            {
                "event name": "Goal", "time": 20,
                "general game updates": {"score": "1-0"},
                "team a updates": {"goals": "1"}, "team b updates": {},
                "description": "Germany scores!"
            }
        ]
    }
    with open(JSON_1, 'w') as f: json.dump(data1, f)
    with open(JSON_2, 'w') as f: json.dump(data2, f)

def cleanup():
    for f in [JSON_1, JSON_2, SUMMARY_LISTENER, SUMMARY_ISOLATED]:
        if os.path.exists(f): os.remove(f)

# --- פונקציה לשליחת פקודה לקליינט ---
def send_cmd(proc, cmd):
    if proc.poll() is not None:
        print_fail(f"Process {proc.pid} is dead! Cannot send command: {cmd}")
        return
    proc.stdin.write(f"{cmd}\n")
    proc.stdin.flush()
    time.sleep(0.5) # השהייה קטנה בין פקודות

# --- המנוע הראשי ---
def run_full_suite():
    cleanup()
    create_test_files()
    
    print_status("Starting Server...")
    server = subprocess.Popen(SERVER_CMD, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(3)

    clients = []
    try:
        # 1. הפעלת 3 לקוחות
        # Reporter: מדווח את האירועים
        # Listener: מקשיב, ואז עושה exit באמצע
        # Isolated: מקשיב לערוץ אחר לגמרי (לוודא שלא מקבל זבל)
        
        print_status("Launching 3 Clients...")
        reporter = subprocess.Popen([CLIENT_PATH], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        listener = subprocess.Popen([CLIENT_PATH], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        isolated = subprocess.Popen([CLIENT_PATH], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        clients = [reporter, listener, isolated]

        # ---------------------------------------------------------
        # בדיקה 1: התחברות וניסיון כפול
        # ---------------------------------------------------------
        print_status("Test 1: Login & Double Login Check")
        send_cmd(reporter, f"login 127.0.0.1:{SERVER_PORT} reporter pass")
        time.sleep(1)
        
        # ננסה להתחבר שוב - הקליינט אמור לחסום או השרת אמור לזרוק
        # אנחנו נבדוק שהקליינט לא קרס
        send_cmd(reporter, f"login 127.0.0.1:{SERVER_PORT} reporter pass") 
        if reporter.poll() is None:
            print_pass("Client handled double-login command without crashing.")
        else:
            print_fail("Client crashed on double login!")

        # חיבור שאר המשתמשים
        send_cmd(listener, f"login 127.0.0.1:{SERVER_PORT} listener pass")
        send_cmd(isolated, f"login 127.0.0.1:{SERVER_PORT} isolated pass")
        time.sleep(1)

        # ---------------------------------------------------------
        # בדיקה 2: הצטרפות לערוצים (Join)
        # ---------------------------------------------------------
        print_status("Test 2: Channel Subscription")
        game_channel = "Germany_Japan"
        other_channel = "Spain_Brazil"

        send_cmd(reporter, f"join {game_channel}")
        send_cmd(listener, f"join {game_channel}") # מאזין לאותו משחק
        send_cmd(isolated, f"join {other_channel}") # מאזין למשחק אחר!
        time.sleep(1)

        # ---------------------------------------------------------
        # בדיקה 3: דיווח ראשון ובידוד (Report Part 1)
        # ---------------------------------------------------------
        print_status(f"Test 3: Reporting Part 1 ({JSON_1})")
        send_cmd(reporter, f"report {JSON_1}")
        time.sleep(2) # זמן הפצה

        # המאזין המבודד לא אמור לקבל כלום. נבדוק את זה בסוף בסיכום שלו.

        # ---------------------------------------------------------
        # בדיקה 4: יציאה מערוץ (Exit / Unsubscribe)
        # ---------------------------------------------------------
        print_status(f"Test 4: Unsubscribe '{listener.pid}' from {game_channel}")
        send_cmd(listener, f"exit {game_channel}") 
        time.sleep(1)

        # ---------------------------------------------------------
        # בדיקה 5: דיווח שני (Report Part 2) - אחרי היציאה
        # ---------------------------------------------------------
        print_status(f"Test 5: Reporting Part 2 ({JSON_2}) - Should NOT reach listener")
        send_cmd(reporter, f"report {JSON_2}")
        time.sleep(2)

        # ---------------------------------------------------------
        # בדיקה 6: הפקת סיכומים (Summary)
        # ---------------------------------------------------------
        print_status("Test 6: Generating Summaries")
        
        # Listener: אמור להכיל את חלק 1, אבל *לא* את חלק 2 (כי הוא עשה exit)
        send_cmd(listener, f"summary {game_channel} reporter {SUMMARY_LISTENER}")
        
        # Isolated: אמור להיות ריק או להכיל שגיאה (כי הוא ב-Spain_Brazil)
        # אבל ננסה לבקש ממנו סיכום על Germany_Japan כדי לראות שלא קיבל כלום
        send_cmd(isolated, f"summary {game_channel} reporter {SUMMARY_ISOLATED}")
        
        time.sleep(1)

        # ---------------------------------------------------------
        # בדיקה 7: ניתוח הקבצים (Verification)
        # ---------------------------------------------------------
        print_status("Test 7: Verifying File Content")
        
        # בדיקת ה-Listener
        if os.path.exists(SUMMARY_LISTENER):
            with open(SUMMARY_LISTENER, 'r') as f:
                content = f.read()
                print(f"{YELLOW}--- Listener Summary ---{RESET}\n{content}\n{YELLOW}------------------------{RESET}")
                
                if "Game Started" in content:
                    print_pass("Event 1 (Before Exit) is present.")
                else:
                    print_fail("Event 1 is MISSING! Join/Report failed.")

                if "Germany scores" not in content:
                    print_pass("Event 2 (After Exit) is correctly ABSENT.")
                else:
                    print_fail("Event 2 found! 'exit' command did NOT work (Update received after unsubscribe).")
        else:
            print_fail("Listener summary file was not created.")

        # בדיקת ה-Isolated
        if os.path.exists(SUMMARY_ISOLATED):
            with open(SUMMARY_ISOLATED, 'r') as f:
                content = f.read()
                if "Game Started" not in content and "Germany scores" not in content:
                     print_pass("Isolated client correctly received NO updates for the wrong channel.")
                else:
                    print_fail("Isolated client received updates for a channel it didn't join!")
        else:
             # זה בסדר אם הקובץ לא נוצר כי אולי הקליינט מדפיס שגיאה לטרמינל במקום ליצור קובץ ריק
             print_pass("Isolated summary not created (Expected, as no data should exist).")

        # ---------------------------------------------------------
        # בדיקה 8: Logout
        # ---------------------------------------------------------
        print_status("Test 8: Logout")
        for c in clients:
            send_cmd(c, "logout")
        time.sleep(1)
        
        # בדיקה שכולם נסגרו (אם המימוש שלך סוגר את הקליינט ב-logout)
        # אם המימוש שלך משאיר את הקליינט פתוח ומחכה ל-login חדש, התהליכים עדיין ירוצו וזה תקין.

    except Exception as e:
        print_fail(f"Exception during test: {e}")

    finally:
        print_status("Cleaning up...")
        for c in clients:
            c.kill()
        server.terminate()
        os.system(f"lsof -ti:{SERVER_PORT} | xargs kill -9 2>/dev/null")
        cleanup()
        print_status("Done.")

if __name__ == "__main__":
    run_full_suite()