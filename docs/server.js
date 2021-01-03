import QuickWebServer from "../src/QuickWebServer.build.js";
import { SERVER_PATH } from './env.js'
import {renderMdPage} from "./utils/renderPage.js";

const app = new QuickWebServer()

app.staticDir('/static', `${SERVER_PATH}/static`)

app.get('/', (req, res) => {
    res.send(renderMdPage('__index'))
})

app.get('/page/:slug', (req, res) => {
    const content = renderMdPage(req.params.slug)
    if (!content) {
        res.redirect('/page-not-found')
    } else {
        res.send(content)
    }
})

app.get('*', (req,res) => {
    res.send(renderMdPage('__404'))
})

app.listen(5432)
