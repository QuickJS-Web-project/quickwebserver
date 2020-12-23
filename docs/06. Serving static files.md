# Serving static files

There is a server.staticDir method for providing 
static files. It takes two arguments:
1. The part of the URI that will be considered 
   the root point of the provided folder;
2. The absolute path to the folder from which 
   you want to serve files to the client.
   
```javascript
server.staticDir('/static', '/var/www/mysite.com/public_html/files');
```

After this command, all paths after static will be 
combined with the folder path. If the file is found, 
the server will read it and give it to the client, 
otherwise a 404 error will be returned.