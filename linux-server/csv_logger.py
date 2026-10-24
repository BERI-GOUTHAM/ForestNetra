#!/usr/bin/env python3

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import csv
import json

HOST = "0.0.0.0"
PORT = 8081

DATA_DIR = Path("/home/min/ForestNetra")
CSV_FILE = DATA_DIR / "forest_netra_data.csv"

COLUMNS = [
    "timestamp_ist", "T", "P", "A", "H", "L", "C", "G",
    "M", "ST", "E", "PH", "N", "PP", "K"
]


class CSVLogger(BaseHTTPRequestHandler):

    def send_json(self, status, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {
                "status": "ok",
                "service": "ForestNetra CSV Logger"
            })
        else:
            self.send_json(404, {"error": "Use /health or POST /log"})

    def do_POST(self):
        if self.path != "/log":
            self.send_json(404, {"error": "Endpoint not found"})
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 16384:
                self.send_json(400, {"error": "Invalid request size"})
                return

            payload = json.loads(self.rfile.read(length))

            row = {
                "timestamp_ist": datetime.now(
                    ZoneInfo("Asia/Kolkata")
                ).isoformat(timespec="seconds")
            }

            for column in COLUMNS[1:]:
                value = payload.get(column)
                if value is None:
                    self.send_json(400, {
                        "error": f"Missing field: {column}"
                    })
                    return

                number = float(value)
                if not __import__("math").isfinite(number):
                    self.send_json(400, {
                        "error": f"Invalid number: {column}"
                    })
                    return
                row[column] = number

            DATA_DIR.mkdir(parents=True, exist_ok=True)
            with CSV_FILE.open("a", newline="", encoding="utf-8") as file:
                writer = csv.DictWriter(file, fieldnames=COLUMNS)
                if file.tell() == 0:
                    writer.writeheader()
                writer.writerow(row)

            self.send_json(200, {
                "status": "saved",
                "file": str(CSV_FILE),
                "timestamp_ist": row["timestamp_ist"]
            })

        except (ValueError, json.JSONDecodeError) as error:
            self.send_json(400, {"error": str(error)})
        except Exception as error:
            print("Logging error:", error, flush=True)
            self.send_json(500, {"error": "Server logging failed"})

    def log_message(self, format, *args):
        print(f"{self.client_address[0]} - {format % args}", flush=True)


if __name__ == "__main__":
    print(f"ForestNetra CSV Logger listening on port {PORT}", flush=True)
    print(f"CSV file: {CSV_FILE}", flush=True)
    ThreadingHTTPServer((HOST, PORT), CSVLogger).serve_forever()