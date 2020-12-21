import { startServer } from '../../libqws.so';
import { Worker } from 'os';

const parent = Worker.parent;

/**
 * Post message to parent process on demand
 * @param {object} data
 */
function serverCallback(data) {
  parent.postMessage({ type: 'request', data: { ...data } });
}

function worker_main() {
  parent.onmessage = (message) => {
    const { type } = message.data;
    switch (type) {
      case 'start_server':
        startServer(serverCallback, message.data.port);
        break;
    }
  };
}

worker_main();
