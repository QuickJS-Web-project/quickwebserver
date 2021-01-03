# Building server
As you know, QuickJS allows you to compile JS code into a 
native application. This means that you can compile your 
server too.

### Compilation requirements
* Web server written on QuickJS + QuickWebServer;
* Your QuickWebServer imports should be a bit modified (read below);
* QuickJS must be installed on your system.

### Modifying imports
The QuickWebServer for runtime is slightly different 
from the version for building the executable. 
Fortunately, you don't have to do a lot of things. 
In all files where you import QuickWebServer, 
you need to change the import as follows:

```javascript
// before: for working in runtime
import QuickWebServer from './node_modules/quickwebserver/src/QuickWebServer.js'

// after: for building an executable
import QuickWebServer from './node_modules/quickwebserver/src/QuickWebServer.build.js'
```
That's all, you can proceed to the next steps.

### Before we begin

Since QWS is a shared library, in order to combine all the 
functionality of QuickJS and the server in a single 
executable file, you need to build your project in 
static linking mode.

This one can be confusing if you haven’t worked with 
building C projects before. This is why QWS provides a 
Makefile generator in which all the build rules and 
parameters are written for you.

### Using the Makefile generator
For example, consider the following working environment. 
Your project is located in the`~/work/qws_server` folder.
You installed QuickWebServer via NPM, so it is 
located in `~/work/qws_server/node_modules/quickwebserver`.

This directory contains the `create-build-makefile.sh` 
file, which is responsible for creating the Makefile. 
Call it with passing the path to the input (main) script 
of your application as an argument.

**Important:** The Makefile will be created in the 
folder where you invoke the command. I recommend 
calling the command in the root directory of your project:

```bash
~/work/qws_server $: ./node_modules/quickwebserver/create-build-makefile.sh ~/work/qws_server/index.js
```

When the command is done, you can call `make` and your 
project will be built. An executable file with the same 
name will appear next to the main file.

This executable is ready to be deployed. No QWS
shared library required.