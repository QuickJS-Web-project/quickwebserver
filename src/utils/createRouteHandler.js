import { regexparam } from './regexparam.js';
import Response from '../response/response.js';
import Request from "../request/request.js";

/**
 * Create handler object for route
 * @param {string} url
 * @param {function} callback
 * @returns {{pathObject, routeHandler: (function(*=): {headers: {}, responseType: string, content: string, status: number}), urlMask}}
 */
export default function createRouteHandler(url, callback) {
  const pathObject = regexparam(url);
  const routeHandler = async (serverData) => {
    const request = new Request(serverData)
    const response = new Response();
    await callback(request, response);
    return response.__responseObject;
  };
  return {
    pathObject,
    urlMask: url,
    routeHandler
  };
}
