import { getType } from '../utils/getType.js';
import mime from '../utils/mime/index.js';

export default class Response {
  headers = {};
  _status = 200;
  content = '';
  responseType = 'string';

  /**
   * Set a response status
   * @param {number} v
   */
  status(v) {
    this._status = v;
  }

  /**
   * A data you want to respond with
   * @param {string|object|array|number} data
   */
  send(data) {
    this.responseType = 'string';
    const dataType = getType(data);
    switch (dataType) {
      case 'object':
      case 'array':
        this.content = JSON.stringify(data);
        break;
      case 'number':
        this.content = String(data);
        break;
      case 'string':
        this.content = data;
        break;
      default:
        this.content = null;
    }
    if (this.content === null) {
      throw new TypeError('Only "string", "object", "array" and "number" supported as response data');
    }
  }

  /**
   * Send file as a response
   * @param {string} path
   */
  sendFile(path) {
    this.responseType = 'file';
    this.type(mime.getType(path));
    this.content = path;
  }

  /**
   * Sets the response’s HTTP header field to value. To set multiple fields at once, pass an object as the parameter
   * @param {string|object} field
   * @param {string} value
   */
  set(field, value) {
    const fieldType = getType(field);
    if (fieldType === 'string') {
      this.headers = {
        ...this.headers,
        [field.toLowerCase()]: value
      };
    } else if (fieldType === 'object') {
      this.headers = {
        ...this.headers,
        ...field
      };
    }
  }

  /**
   * Sets the Content-Type HTTP header to the MIME type
   * @param {string} mime
   */
  type(mime) {
    this.headers = {
      ...this.headers,
      'content-type': mime
    };
  }

  get __responseObject() {
    return {
      status: this._status,
      headers: this.headers,
      content: this.content,
      responseType: this.responseType
    };
  }
}
