#!/bin/bash

# --- הגדרות וצבעים ---
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Starting Full Automation Test (SQL + Java + C++) ===${NC}"

# 1. ניקוי תהליכים ישנים כדי למנוע שגיאות "Address already in use"
echo "Cleaning up old processes..."
fuser -k 7778/tcp 2>/dev/null # סגירת שרת הפיסון
fuser -k 2000/tcp 2>/dev/null # סגירת שרת הג'אווה
rm -f stomp_server.db sql_output.log # מחיקת ה-DB הקודם להתחלה נקייה

# 2. הפעלת שרת ה-SQL (Python) ברקע
echo "Step 1: Starting Python SQL Server on port 7778..."
python3 data/sql_server.py 7778 > sql_output.log 2>&1 &
PYTHON_PID=$!
sleep 2 # מחכה שהשרת יעלה

# 3. הפעלת שרת ה-STOMP (Java) ברקע
echo "Step 2: Starting Java SPL Server on port 2000..."
# הפקודה מריצה את השרת בגרסת ה-TPC (ניתן לשנות ל-reactor אם רוצים)
mvn -f server/pom.xml exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="2000 tpc" > /dev/null 2>&1 &
JAVA_PID=$!
echo "Waiting 5 seconds for Java server to initialize..."
sleep 5

# 4. הפעלת לקוח ה-C++ ושליחת פקודות אוטומטית
echo -e "${BLUE}Step 3: Running C++ Client with commands...${NC}"
# אנחנו משתמשים ב-EOF כדי להזרים פקודות לתוך הלקוח בלי להקליד ידנית
./client/bin/StompClient << 'EOF'
login 127.0.0.1:2000 alice pass123
join germany_israel
report client/data/events1.json
logout
EOF

echo "Commands sent. Waiting for DB to update..."
sleep 2

# 5. שליחת פקודת ה-REPORT לשרת הפיסון להצגת התוצאות
echo -e "${GREEN}Step 4: Triggering the SQL REPORT...${NC}"
# הפקודה nc שולחת את המילה REPORT עם ה-Null Terminator לפורט 7778
echo -n -e "REPORT\0" | nc -w 1 127.0.0.1 7778 > /dev/null

# 6. הצגת הפלט הסופי מתוך הלוג של שרת הפיסון
echo -e "\n${BLUE}================ ACTUAL SQL DATABASE REPORT ================${NC}"
# מחפש את תחילת הדו"ח בקובץ הלוג ומדפיס 20 שורות קדימה
grep -A 20 "--- SERVER SQL REPORT ---" sql_output.log
echo -e "${BLUE}============================================================${NC}"

# 7. סגירת השרתים בסיום
echo "Shutting down servers..."
kill $PYTHON_PID
kill $JAVA_PID

echo -e "${GREEN}Test Finished Successfully!${NC}"