import http.server as server
import socketserver

class CORSHTTPRequestHandler(server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_CORS_headers()

        server.SimpleHTTPRequestHandler.end_headers(self)

    def send_CORS_headers(self):
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Access-Control-Allow-Origin', '*')

def start_local_server():
    PORT = 3000

    Handler = CORSHTTPRequestHandler
    Handler.extensions_map['.wasm'] = 'application/wasm'
    Handler.extensions_map['.js'] = 'application/javascript'
    Handler.extensions_map['.map'] = 'application/json'

    httpd = socketserver.TCPServer(("", PORT), Handler)

    print("serving at port",PORT)
    httpd.serve_forever()

start_local_server()