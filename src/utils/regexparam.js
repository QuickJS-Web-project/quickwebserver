export { parse as regexparam } from '../deps/regexparam.js';
export function exec(url, pathObject) {
  let i = 0,
    out = {};
  let matches = pathObject.pattern.exec(url);
  while (i < pathObject.keys.length) {
    out[pathObject.keys[i]] = matches[++i] || null;
  }
  return out;
}
