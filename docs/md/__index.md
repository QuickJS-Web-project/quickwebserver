# QuickWebServer
### Implementation of HTTP web server in the QuickJS Runtime

**[QuickJS](https://bellard.org/quickjs/)** (&copy; Fabrice Bellard and Charlie Gordon) is a small
and embeddable Javascript engine. It supports the ES2020 specification including modules,
asynchronous generators, proxies and BigInt.

This repository is an HTTP web server implementation for QuickJS. It consists of two parts:
1. A shared C library that adds a native module to QuickJS to start a web server
   and handle requests (uses [httpserver.h](https://github.com/jeremycw/httpserver.h) by Jeremy Williams);
2. JavaScript API for easy interaction with this module, inspired by Express.js

### What the server can do now
* Handle GET and POST requests;
* Provide headers, url and request body;
* Process static files, serve them with the correct MIME type;
* Provides a convenient API for response: setting headers, content type, sending files.

### Is the server ready for production?
Not really. Testing was carried out on macOS and Ubuntu, load tests have not been performed yet.
A stable version has not yet been released.

### How can I use it now?
QuickJS gives us several benefits:
* Tiny size of the engine;
* Relatively high speed of work ([benchmarks](https://bellard.org/quickjs/bench.html));
* Ability to compile your JS application into a native binary application.

Thus, you can use QuickJS and QuickWebServer to build simple microservices, APIs, or serve
static sites.

Ultimately, most of the work happens outside the server's area of responsibility
(for example, working with databases).

### TODOs and known issues
- [ ] No parsing of the request body, only raw content. Solvable in the case of `json`, but
  `form-data` and `x-www-form-urlencoded` are inconvenient;
- [ ] File uploading via POST `multipart/form-data` doesn't work yet;
- [ ] Sending a POST request with a file larger than 1MB may cause a server crash;

_You can add to this list by opening an issue in repository._
