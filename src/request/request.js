import parseQuery from '../utils/parseQuery.js';

export default class Request {
  httpData = {};

  /**
   * Getting raw data from server; formatting all
   * to lower case
   *
   * @param {object} httpData
   */
  constructor(httpData) {
    for (const key in httpData) {
      if (httpData.hasOwnProperty(key)) {
        this.httpData[key.toLowerCase()] = httpData[key];
      }
    }
    for (const key in httpData.headers) {
      if (httpData.headers.hasOwnProperty(key)) {
        const value = httpData.headers[key];
        delete this.httpData.headers[key];
        this.httpData.headers[key.toLowerCase()] = value;
      }
    }
  }

  /**
   * Get raw body string
   *
   * @returns {string}
   */
  get body() {
    const contentType = this.headers['content-type'] ?? 'application/json';
    let returnData;
    switch (contentType) {
      case 'application/x-www-form-urlencoded':
        try {
          returnData = parseQuery(this.httpData.body);
        } catch (e) {
          returnData = null;
        }
        break;

      case 'application/json':
      default:
        try {
          returnData = JSON.parse(this.httpData.body);
        } catch (e) {
          returnData = null;
        }
        break;
    }
    if (contentType.includes('multipart/form-data')) {
      // @todo: parse multipart/form-data
      console.log('[QuickWebServer] parsing multipart/form-data is not supported for now');
    }
    return returnData ?? this.httpData.body;
  }

  /**
   * Parsing request cookies to key-value object
   *
   * @returns {object}
   */
  get cookies() {
    const cookies = this.httpData.headers?.cookie ?? '';
    if (!cookies) return {};
    return cookies.split(';').reduce((total, current) => {
      const [key, value] = current.split('=');
      total[key.trim()] = value.trim();
      return total;
    }, {});
  }

  /**
   * Get request hostname
   *
   * @returns {string|null}
   */
  get hostname() {
    return this.httpData.headers?.host ?? null;
  }

  /**
   * Get request method
   *
   * @returns {string}
   */
  get method() {
    return this.httpData.method;
  }

  /**
   * Get request path params
   *
   * @returns {object}
   */
  get params() {
    return this.httpData.params;
  }

  /**
   * Get request relative url
   *
   * @returns {string}
   */
  get path() {
    return this.httpData.url;
  }

  /**
   * Get request query params
   *
   * @returns {object}
   */
  get query() {
    const [path, query] = this.httpData.url.split('?');
    if (!query) return {};
    return parseQuery(query);
  }

  /**
   * Get request headers
   *
   * @return {object}
   */
  get headers() {
    return this.httpData.headers;
  }

  /**
   * Get specific header by name
   *
   * @param {string} headerKey
   * @return {string|undefined}
   */
  get(headerKey) {
    return this.httpData.headers?.[headerKey] ?? undefined;
  }
}
