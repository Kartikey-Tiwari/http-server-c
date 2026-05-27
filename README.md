# HTTP Server in C

A small, dependency-free TCP-to-HTTP server in C. Provides a minimal HTTP parser, response builder and a pluggable request handler so you can run a static file server or implement custom API handlers.

**Key features**

- **Small footprint:** single-process, minimal dependencies.
- **Pluggable handler:** pass a function of type `void (*handler)(Request *req, Response *res)` to `serverListen()` to control request handling.
- **Static file mode:** built-in `static_file_handler` serves a provided folder (e.g. `./public`).

**Build**

Run:

```sh
make
```

This produces the `server` binary.

**Run**

- API mode (default):

```sh
./server --port 8080
```

- Static-file mode (serve `./public`):

```sh
./server --static ./public --port 8080
```

The binary name and build target are defined in the `Makefile` (target `server`).

**How the handler is used**

The server accepts a handler with this signature:

```c
void (*handler)(Request *req, Response *res)
```

See [server.c](server.c) for the `serverListen()` implementation that invokes the handler for each successfully parsed request.

The repository provides two handler examples:

- The default `handler` in [main.c](main.c) — a minimal example that returns `200 OK` for `/` and `404 Not Found` otherwise.
- `static_file_handler` in [utils.c](utils.c) — serves files from the folder passed via `--static`.

**Handler use cases & examples**

1. Minimal root handler (already in [main.c](main.c)) — returns simple HTML.

```c
void handler(Request *req, Response *res) {
  if (strcmp(req->rql.resource->str, "/") == 0) {
    writeStatusLine(res, OK);
    setHeader(res->headers, d_str_new("Content-Type"), d_str_new("text/html"));
    char *body = httpCodeToHTML(OK);
    setHeader(res->headers, d_str_new("Content-Length"), d_str_new(itoa(strlen(body))));
    writeHeaders(res);
    writeBody(res, body, strlen(body));
    responseEnd(res);
  } else {
    // Not found fallback
    writeStatusLine(res, NOT_FOUND);
    // ... write a simple body, headers, then responseEnd(res)
  }
}
```

2. JSON API endpoint example — simple routing and JSON response

```c
void handler(Request *req, Response *res) {
  const char *path = req->rql.resource->str;
  if (strcmp(path, "/api/health") == 0) {
    const char *body = "{\"status\":\"ok\"}";
    writeStatusLine(res, OK);
    setHeader(res->headers, d_str_new("Content-Type"), d_str_new("application/json"));
    char *len = itoa(strlen(body));
    setHeader(res->headers, d_str_new("Content-Length"), d_str_new(len));
    free(len);
    writeHeaders(res);
    writeBody(res, (char *)body, strlen(body));
    responseEnd(res);
    return;
  }
  // Delegate to default behavior or return 404
  writeStatusLine(res, NOT_FOUND);
  writeHeaders(res);
  responseEnd(res);
}
```

3. Use the built-in static file handler

The server supports serving a folder by calling `serverListen()` with `static_file_handler` or by using the `--static` CLI flag. Example:

```c
Server *srv = createServer(8080);
serverListen(srv, static_file_handler);
```

Or from the command line:

```sh
./server --static ./public
```

**Sample curl requests**

Use these examples against a locally running server on port `8080` (adjust the port if you started the server with `--port`).

- Basic GET for the root endpoint:

```sh
curl -i http://localhost:8080/
```

- Health-check JSON endpoint (example handler `/api/health`):

```sh
curl -i http://localhost:8080/api/health
```

- Fetch a static file (when `--static ./public` is used):

```sh
curl -i http://localhost:8080/index.html
```

- POST JSON to a custom API path (handler must read the request body):

```sh
curl -i -X POST -H "Content-Type: application/json" -d '{"name":"alice"}' http://localhost:8080/api/data
```
