"""Serve a built Web SDK example on loopback, without requiring COOP/COEP."""
import argparse
import re
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

class Handler(SimpleHTTPRequestHandler):
    extensions_map={**SimpleHTTPRequestHandler.extensions_map,'.wasm':'application/wasm','.js':'text/javascript'}
    def end_headers(self):
        # Content-addressed assets are immutable; HTML/catalogues and fixed-name WASM revalidate.
        hashed=bool(re.search(r'/r\d+-[0-9a-f]{16}-[^/]+$',urlsplit(self.path).path))
        self.send_header('Cache-Control','public, max-age=31536000, immutable' if hashed else 'no-cache')
        super().end_headers()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,required=True)
    parser.add_argument('--port',type=int,default=8088)
    args=parser.parse_args();directory=args.root.resolve(strict=True)
    if not directory.is_dir():parser.error('--root must be a built Web directory')
    server=ThreadingHTTPServer(('127.0.0.1',args.port),partial(Handler,directory=str(directory)))
    print(f'Epidemic Web: http://127.0.0.1:{args.port}/',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()
