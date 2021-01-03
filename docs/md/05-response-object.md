# Response object

The response object helps you control the headers 
and data sent to the client. List of methods and properties:

* `response.status(status: number)`  
Sets the HTTP status with which the response will be sent.

* `response.set(field: string | object [, value: string])`    
Controls response headers. The first argument can be 
an object, and then the headers corresponding to the 
keys and values will be passed, or it can be a string. 
In this case, the second argument will be the header 
value.    
Example:  
```javascript
response.set({
    'Content-Type': 'application/json'
});  
// or
response.set('Content-Type', 'application/json');
```

* `response.type(type: string)`   
Sets the Content-Type header of the response

* `response.send(data: string|number|object|array)`   
Passing data to the response body. If an array or 
object is passed, the data will be converted to 
a string using `JSON.stringify`.

* `response.redirect(url: string)`   
301 redirect to `url`

* `response.sendFile(path: string)`   
Sends the file at `path` to the client. The MIME 
type of the response is automatically set.