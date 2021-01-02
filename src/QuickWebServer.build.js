import * as WebServer from 'quickwebserver';
import { setTimeout, Worker } from 'os';
import { HTTP_NOT_FOUND } from './data/httpErrors.js';
import createRouteHandler from './utils/createRouteHandler.js';
import { findRouteHandler } from './utils/findRouteHandler.js';
import { deleteFirstSlash, deleteLastSlash, deleteEdgeSlashes } from './utils/deleteSlashes.js';
import mime from './utils/mime/index.js';

export default class QuickWebServer {
  worker = new Worker('./worker/server_worker.build.js');

  routeHandlers = {
    GET: [],
    POST: []
  };

  /**
   * Start listening to connections
   * @param {number} port
   */
  listen(port = 3000) {
    this.worker.onmessage = this.workerMessage.bind(this);
    try {
      this.tick();
      this.worker.postMessage({ type: 'start_server', port });
    } catch (e) {
      console.log(e);
      exit(1);
    }
  }

  /**
   * Handling message from Worker (from C module)
   * @param {object} data
   * @returns {Promise<void>}
   */
  async workerMessage(data) {
    const messageType = data.data.type;
    if (messageType === 'request') {
      const { http: httpData, requestId } = data.data.data;
      const url = httpData.url;
      const routeHandler = findRouteHandler(url, this.routeHandlers[httpData.method], httpData);
      if (!routeHandler) {
        WebServer.respond(requestId, HTTP_NOT_FOUND);
      } else {
        WebServer.respond(requestId, await routeHandler(httpData));
      }
    }
  }

  /**
   * Internal server loop
   */
  tick() {
    setTimeout(() => {
      this.tick();
    }, 1000);
  }

  /**
   * Serving static files on url
   * @param {string} urlAlias
   * @param {string} path
   */
  staticDir(urlAlias, path) {
    urlAlias = '/' + deleteEdgeSlashes(urlAlias) + '/*';
    this.routeHandlers.GET.push(
      createRouteHandler(urlAlias, async (serverData, response) => {
        const { wild } = serverData.params;
        if (!wild) {
          response.status(404);
          response.send('Not found');
          return;
        }
        const fullPath = `${deleteLastSlash(path)}/${deleteFirstSlash(wild)}`;
        response.type(mime.getType(fullPath));
        response.sendFile(fullPath);
      })
    );
  }

  /**
   * Attach GET route handler for specific URL
   * @param {string} url
   * @param {function} callback
   */
  get(url, callback) {
    this.routeHandlers.GET.push(createRouteHandler(url, callback));
  }

  /**
   * Attach POST route handler for specific URL
   * @param {string} url
   * @param {function} callback
   */
  post(url, callback) {
    this.routeHandlers.POST.push(createRouteHandler(url, callback))
  }
}
