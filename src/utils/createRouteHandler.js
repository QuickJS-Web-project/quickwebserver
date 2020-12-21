import {regexparam} from "./regexparam.js";
import {createEmptyResponse} from "../response/createResponse.js";

export default function createRouteHandler(url, callback) {
    const pathObject = regexparam(url)
    const routeHandler = async (serverData) => {
        const response = createEmptyResponse()
        await callback(serverData, response)
        return response.__responseObject
    }
    return  {
        pathObject,
        urlMask: url,
        routeHandler
    }
}