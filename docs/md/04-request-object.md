# Request object
Request object methods allow you to retrieve data sent 
by the client. List of methods and properties:

* `request.body (getter)`  
Returns a raw string from the request body. **Note**: in the current version, the data is not 
parsed in any way. Regardless of the content type of 
the request, a raw request body string is returned. If 
JSON was passed in the request, you can process it 
using `JSON.parse`.

* `request.cookies (getter)`  
Returns an object (key-value) with the cookies passed in the request.

* `request.hostname (getter)`  
Returns the hostname derived from the Host HTTP header.

* `request.method (getter)`  
Contains a string corresponding to the HTTP method 
of the request: GET or POST.

* `request.params (getter)`  
Object with parameter values from route placeholders.
Read more in "Routing" section.

* `request.path (getter)`  
URI of request

* `request.query (getter)`  
Object with query parameters. For example, if you have 
a request with query string `?key=value&key2=value2`, 
request.query will return:
```javascript
{
    key: 'value',
    key2: 'value2'
}
```

* `request.headers (getter)`  
Object with request HTTP headers (full list).

* `request.get(key: string)`  
Returns header value by key (or undefined).
