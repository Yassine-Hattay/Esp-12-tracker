import http.server
import socketserver
import os

# ==============================
# CONFIGURATION
# ==============================
PORT = 8000
FIRMWARE_FILE = "hello-world1.bin"
SERVE_DIR = r"C:\msys32\home\hatta\esp\tracker\build"
# ==============================

os.chdir(SERVE_DIR)

class OTARequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.endswith(FIRMWARE_FILE):
            print(f"[OTA] Sending firmware: {FIRMWARE_FILE}")
        return super().do_GET()

with socketserver.TCPServer(("", PORT), OTARequestHandler) as httpd:
    print(f"[OTA] Serving {FIRMWARE_FILE} at http://{httpd.server_address[0]}:{PORT}/{FIRMWARE_FILE}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[OTA] Server stopped.")
        httpd.server_close()
