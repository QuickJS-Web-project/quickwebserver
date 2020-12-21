/**
 * All credits goes to Vue Router
 * https://github.com/vuejs/vue-router/blob/dev/src/util/query.js
 */

/**
 *
 * @param str
 * @return {string|*}
 */
function decode(str) {
    try {
        return decodeURIComponent(str)
    } catch (err) {
        console.warn(`[QuickWebServer] Could not decode query string: ${str}`)
    }
    return str
}

/**
 *
 * @param query
 * @return {object}
 */
export default function parseQuery(query) {
    const res = {}
    if (typeof query !== 'string') return res

    query = query.trim().replace(/^(\?|#|&)/, '')

    if (!query) {
        return res
    }

    query.split('&').forEach((param) => {
        const parts = param.replace(/\+/g, ' ').split('=')
        const key = decode(parts.shift())
        const val = parts.length > 0 ? decode(parts.join('=')) : null

        if (res[key] === undefined) {
            res[key] = val
        } else if (Array.isArray(res[key])) {
            val !== null && res[key].push(val)
        } else {
            if (val !== null) {
                res[key] = [res[key], val]
            }
        }
    })

    return res
}