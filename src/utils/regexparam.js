export { default as regexparam } from '../../node_modules/regexparam/dist/regexparam.mjs'
export function exec(url, pathObject) {
    let i=0, out={};
    let matches = pathObject.pattern.exec(url);
    while (i < pathObject.keys.length) {
        out[ pathObject.keys[i] ] = matches[++i] || null;
    }
    return out;
}