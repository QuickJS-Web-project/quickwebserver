/**
 * Copyright: https://github.com/broofa/mime
 */

import Mime from './Mime.js'
import typesStandard from './standard.js'
import typesOther from './other.js'
export default new Mime(typesStandard, typesOther);