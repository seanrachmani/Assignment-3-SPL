#!/usr/bin/env python3
"""
Basic Python Server for STOMP Assignment – Stage 3.3

IMPORTANT:
DO NOT CHANGE the server name or the basic protocol.
Students should EXTEND this server by implementing
the methods below.
"""

import socket
import sys
import threading
import sqlite3


SERVER_NAME = "STOMP_PYTHON_SQL_SERVER"  # DO NOT CHANGE!
DB_FILE = "stomp_server.db"              # DO NOT CHANGE!


def recv_null_terminated(sock: socket.socket) -> str:
    data = b""
    while True:
        chunk = sock.recv(1024)
        if not chunk:
            return ""
        data += chunk
        if b"\0" in data:
            msg, _ = data.split(b"\0", 1)
            return msg.decode("utf-8", errors="replace")


_conn = sqlite3.connect(DB_FILE)
cursor = _conn.cursor()

def init_database():
    _conn.executescript("""
                         
            CREATE TABLE users (
            username    TEXT        PRIMARY KEY,
            user_id      INT         NOT NULL,
            password    TEXT        NOT NULL
        );
                             
        CREATE TABLE login_history (
            id          INTEGER     PRIMARY KEY,
            username    TEXT        NOT NULL,
            login_time  DATETIME    NOT NULL,
            logout_time DATETIME    
                         
           FOREIGN KEY(username) REFERENCES users(username)              
        );
                         
        CREATE TABLE file_tracking (
            file_id     INTEGER     PRIMARY KEY,
            file_name   TEXT        NOT NULL,
            username_of_submitter   TEXT,
            game_channel            TEXT        NOT NULL,      
            timestamp               DATETIME    NOT NULL,
                         
            FOREIGN KEY(username_of_submitter) REFERENCES users(username)
        );
    """)
    _conn.commit()



def execute_sql_command(sql_command: str) -> str:
    _conn.execute(sql_command)



def execute_sql_query(sql_query: str) -> str:
    cursor = _conn.cursor()
    cursor.execute(sql_query)
    output = cursor.fetchall()
    return str(output)

def Report():
    print ( "------REPORT FROM SQL-----")
    all_users = _conn.execute ("SELECT username FROM users")
    for user in 


def handle_client(client_socket: socket.socket, addr):
    print(f"[{SERVER_NAME}] Client connected from {addr}")

    try:
        while True:
            message = recv_null_terminated(client_socket)
            if message == "":
                break

            print(f"[{SERVER_NAME}] Received:")
            print(message)

            client_socket.sendall(b"done\0")

    except Exception as e:
        print(f"[{SERVER_NAME}] Error handling client {addr}: {e}")
    finally:
        try:
            client_socket.close()
        except Exception:
            pass
        print(f"[{SERVER_NAME}] Client {addr} disconnected")


def start_server(host="127.0.0.1", port=7778):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((host, port))
        server_socket.listen(5)
        print(f"[{SERVER_NAME}] Server started on {host}:{port}")
        print(f"[{SERVER_NAME}] Waiting for connections...")

        while True:
            client_socket, addr = server_socket.accept()
            t = threading.Thread(
                target=handle_client,
                args=(client_socket, addr),
                daemon=True
            )
            t.start()

    except KeyboardInterrupt:
        print(f"\n[{SERVER_NAME}] Shutting down server...")
    finally:
        try:
            server_socket.close()
        except Exception:
            pass


if __name__ == "__main__":
    port = 7778
    if len(sys.argv) > 1:
        raw_port = sys.argv[1].strip()
        try:
            port = int(raw_port)
        except ValueError:
            print(f"Invalid port '{raw_port}', falling back to default {port}")

    start_server(port=port)
