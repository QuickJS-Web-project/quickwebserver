import { exec } from './regexparam.js';

export function findRouteHandler(url, handlers, httpData) {
  const routeObject = handlers.find((handler) => {
    return handler.pathObject.pattern.test(url);
  });
  if (!routeObject) return null;
  const pathData = exec(url, routeObject.pathObject);
  httpData['params'] = pathData;
  return routeObject.routeHandler;
}
