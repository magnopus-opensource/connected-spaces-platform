import http.server as server
import socketserver
import sys


SERVER_PORT = 8080
WEB_DIRECTORY = 'html'


class CORSHTTPRequestHandler(server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIRECTORY, **kwargs)

    def end_headers(self):
        self.send_CORS_headers()

        server.SimpleHTTPRequestHandler.end_headers(self)

    def send_CORS_headers(self):
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Access-Control-Allow-Origin', '*')

    # Shut up log messages
    def log_message(self, format, *args):
        pass


class QuietTCPServer(socketserver.TCPServer):
    def handle_error(self, request, client_address) -> None:
        err = sys.exc_info()[1][0]

        # Shut up error message caused by remote client disconnecting
        if err != 10054:
            super().handle_error(request, client_address)


class LocalHttpServer:
    httpd: socketserver.TCPServer

    def start(self):
        Handler = CORSHTTPRequestHandler
        Handler.extensions_map['.wasm'] = 'application/wasm'
        Handler.extensions_map['.js'] = 'application/javascript'
        Handler.extensions_map['.map'] = 'application/json'

        self.httpd = QuietTCPServer(("", SERVER_PORT), Handler)

        print("serving at port", SERVER_PORT)

        try:
            self.httpd.serve_forever()
        except Exception:
            self.httpd.shutdown()

    def stop(self):
        self.httpd.shutdown()