import { startServer } from '/Users/alexeysolovjov/CODE/Github/qjs-web/webserver.shared.so'
import * as os from "os";

const parent = os.Worker.parent;

function serverCallback(data) {
    parent.postMessage({ type: 'request', data: { ...data } })
}

function worker_main(params) {
    parent.onmessage = (message) => {
        const { type } = message.data
        switch (type) {
            case 'start_server':
                startServer(serverCallback, message.data.port)
                break
        }
    }
}

worker_main()