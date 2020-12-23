# Routing
QWS routing is similar to Express routing.

Each route handler is responsible for a specific URI. 
You can specify dynamic parts of the route, then they 
can be used to receive data.

For example, for route `/user/:userId` with the URI 
`/user/1337` you will get this:

```javascript
server.get('/user/:userId', (request, response) => {
    console.log(request.params)
    // { userId: '1337 }
})
```

The maximum data will be selected from the request 
URI: path parameters, query parameters.

Route paths, in combination with a request method, 
define the endpoints at which requests can be made. 
Route paths can be strings or string patterns.

https://github.com/lukeed/regexparam is used for 
routes matching. Read more about path matching there.

