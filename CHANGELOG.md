### v0.0.2
* **feat**: automated body parsing for `application/json`
and `application/x-www-form-urlencoded`;
* **fix**: better headers string slice
  (fixed problem with empty `Content-Type` key);
* **fix**: return completely raw body from HTTP parsing;
* **fix**: deleting request from requests array after
completion (C);
* **refactor**: C array management rewritten (thanks to https://github.com/andrei-markeev/ts2c);
* **refactor**: some C util functions are in separate file now.
