import {exec} from './regexparam.js';

export function findRouteHandler(url, handlers, httpData) {
  const [path] = url.split('?')
  const routeObject = handlers.find((handler) => {
    return handler.pathObject.pattern.test(path);
  });
  if (!routeObject) return null;
  httpData['params'] = exec(url, routeObject.pathObject);
  return routeObject.routeHandler;
}
