from http import server
from http.server import HTTPServer, BaseHTTPRequestHandler, SimpleHTTPRequestHandler
from pathlib import Path


PORT = 80


class PiCertServerDir(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        self.cert_path = Path("certs") / "picontrol.pem"
        kwargs.update(dict(directory=self.cert_path.parent))
        super().__init__(*args, **kwargs)

def start_server():
    try:
        server = HTTPServer(('', PORT), PiCertServerDir)
        print(f"Starting the server at port {PORT}")
        server.serve_forever()
    except KeyboardInterrupt:
        print(f"Closing...")

if __name__ == "__main__":
    start_server()
