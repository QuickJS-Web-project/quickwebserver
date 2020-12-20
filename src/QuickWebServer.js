import * as WebServer from '/Users/alexeysolovjov/CODE/Github/qjs-web/webserver.shared.so'
import { setTimeout, Worker } from 'os'
import {HTTP_NOT_FOUND} from "./data/httpErrors.js";
import createRouteHandler from "./utils/createRouteHandler.js";
import {findRouteHandler} from "./utils/findRouteHandler.js";
import {deleteFirstSlash, deleteLastSlash} from "./utils/deleteSlashes.js";
import mime from './utils/mime/index.js'

export class QuickWebServer {
    worker = new Worker('./worker/server_worker.js')

    routeHandlers = {
        GET: [],
        POST: []
    }

    listen(port = 3000) {
        this.worker.onmessage = this.workerMessage.bind(this)

        try {
            this.tick()
            this.worker.postMessage({ type: 'start_server', port: 3000 })
        } catch (e) {
            console.log(e)
            exit(1)
        }
    }

    async workerMessage(data) {
        const messageType = data.data.type
        if (messageType === 'request') {
            const { http: httpData, requestId } = data.data.data
            const url = httpData.url
            const routeHandler = findRouteHandler(url, this.routeHandlers[httpData.method], httpData)
            if (!routeHandler) {
                WebServer.respond(requestId, HTTP_NOT_FOUND)
            } else {
                WebServer.respond(requestId, await routeHandler(httpData))
            }
        }
    }

    tick() {
        setTimeout(() => {
            this.tick()
        }, 1000)
    }

    /**
     * @description Serving static files on url
     * @param {string} urlAlias
     * @param {string} path
     */
    staticDir(urlAlias, path) {
        this.routeHandlers.GET.push(createRouteHandler(urlAlias + '*', async (serverData, response) => {
            const { wild } = serverData.params
            if (!wild) {
                response.status(404)
                response.send('Not found')
                return
            }
            const fullPath = `${deleteLastSlash(path)}/${deleteFirstSlash(wild)}`
            response.type(mime.getType(fullPath))
            console.log(mime.getType(fullPath))
            response.sendFile(fullPath)
        }))
    }

    get(url, callback) {
        this.routeHandlers.GET.push(createRouteHandler(url, callback))
    }
}