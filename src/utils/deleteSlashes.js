export const deleteLastSlash = (url) => url.replace(/\/$/, '');
export const deleteFirstSlash = (url) => url.replace(/^\//, '');
export const deleteEdgeSlashes = (url) => deleteFirstSlash(deleteLastSlash(url));
