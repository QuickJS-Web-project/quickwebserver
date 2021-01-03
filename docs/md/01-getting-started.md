# Getting started 

**QuickWebServer (QWS)** is intended to be used as a separately compiled shared library.

The compilation process is as automated as possible, a minimum of steps is required from 
you. Compilation tested on macOS Catalina and Ubuntu 18.04. The `clang` compiler is used 
on macOS, `gcc` on Ubuntu.

## Requirements 
* Compiler availability depending on OS (`clang` or `gcc`)
* Installed QuickJS (minimum version is `2020-11-08`)
* `tar` and `curl`

## Installing via NPM
QuickJS doesn't support NPM, but you can still use it to download everything you need. To 
do this, in the folder with your project, type:

```bash
npm install --save quick-web-server
```

After downloading, a postinstall script will run that compiles 
the C module. This process can take a long time (more than 5 
minutes). Once the compilation is complete (and if there are 
no errors), you can use the server.

## Installing manually
This method differs from the previous method in that you need 
to perform a little more actions:
1. Clone this repository into a convenient folder;
2. Go to it and run the script `./build-shared.sh`. This will start the process of compiling the C module;
3. When compilation is complete (and if no errors occurred), you can use the server.
   
## Adding to the project
There is a QuickWebServer.js file inside the src folder. Its 
default export is the server class we will be working with.