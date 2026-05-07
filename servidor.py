#!/usr/bin/env python3
"""
Servidor HTTP do AdaptaProvas.

Serve os arquivos estaticos da pasta web/ e atua como PROXY para a API
da Pollinations.ai. O proxy e necessario porque a Pollinations passou a
retornar avisos de deprecacao quando o request vem com header Origin
(ou seja, vindo de um navegador). O servidor faz a requisicao server-side
sem esse header, e a API funciona normalmente.

Uso: python servidor.py [porta]
"""

import http.server
import socketserver
import json
import os
import sys
import urllib.request
import urllib.error

PORTA = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
WEB_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "web")

# Endpoints que o servidor expoe como proxy
PROXY_TARGETS = {
    "/api/pollinations": "https://text.pollinations.ai/openai",
}


class AdaptaProvasHandler(http.server.SimpleHTTPRequestHandler):
    """Handler que serve arquivos web/ e faz proxy para APIs de IA."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    # Desabilita cache para que mudancas no JS apareçam imediatamente
    def end_headers(self):
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()

    def log_message(self, fmt, *args):
        # Log mais limpo
        sys.stderr.write("  [server] %s\n" % (fmt % args))

    def do_OPTIONS(self):
        """Responde a CORS preflight de qualquer origem."""
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
        self.end_headers()

    def do_POST(self):
        """Roteia POSTs: se for /api/*, faz proxy. Senao, 404."""
        path = self.path.split("?", 1)[0].rstrip("/")
        target = PROXY_TARGETS.get(path)

        if target is None:
            self.send_error(404, "Endpoint nao encontrado")
            return

        # Le o body do request
        try:
            content_length = int(self.headers.get("Content-Length", 0))
        except (TypeError, ValueError):
            content_length = 0
        body = self.rfile.read(content_length) if content_length else b""

        # Repassa para o destino, SEM o header Origin que faz a Pollinations
        # tratar o request como "navegador" e retornar deprecacao.
        req = urllib.request.Request(
            target,
            data=body,
            method="POST",
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json",
                "User-Agent": "AdaptaProvas/1.0 (Python proxy)",
            },
        )

        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                resp_body = resp.read()
                resp_status = resp.status
                resp_ct = resp.headers.get("Content-Type", "application/json")
        except urllib.error.HTTPError as e:
            resp_body = e.read() if e.fp else b'{"error":"upstream error"}'
            resp_status = e.code
            resp_ct = "application/json"
        except urllib.error.URLError as e:
            self.send_response(502)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            err = json.dumps({"error": f"Falha de rede: {str(e.reason)}"}).encode("utf-8")
            self.wfile.write(err)
            return

        # Devolve a resposta da API ao navegador
        self.send_response(resp_status)
        self.send_header("Content-Type", resp_ct)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(resp_body)))
        super().end_headers()  # ja envia headers padrao
        self.wfile.write(resp_body)


def main():
    handler = AdaptaProvasHandler
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("127.0.0.1", PORTA), handler) as httpd:
        print(f"  [servidor] AdaptaProvas rodando em http://127.0.0.1:{PORTA}")
        print(f"  [servidor] Pasta web: {WEB_DIR}")
        print(f"  [servidor] Proxy de IA: /api/pollinations -> text.pollinations.ai/openai")
        print(f"  [servidor] (Ctrl+C para encerrar)")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n  [servidor] Encerrando...")


if __name__ == "__main__":
    main()
