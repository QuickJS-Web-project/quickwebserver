# Defining routes

Let's create an instance of the class imported in the 
previous step:

```javascript
import QuickWebServer from "./node_modules/quick_web_server/src/QuickWebServer.js";

const server = new QuickWebServer();
```

Now we can add route handlers. If you are familiar with 
Express for Node.js, this step will be familiar to you.

**Note:** In the current version of the server, **only the GET and POST** methods are available.

```javascript
server.get('/', (request, response) => {
    response.send('Hello from QuickJS!');
});

server.post('/post', (request, response) => {
    response.send('Post: hello from QuickJS!');
})
```
Each handler is a function with two arguments. The first is 
the URL for which the handler is responsible, the second is 
the callback, in which the request and response objects are 
available, in which you can produce any data and respond to 
the request.

The callback can be an asynchronous function. Read more about 
request and response objects in the corresponding sections.

# Starting the server

You can start accepting connections using the 
`server.listen` method. The default port is `3000`, you 
can change it by passing a numeric argument to the 
method.

```javascript
server.listen(8080)
```