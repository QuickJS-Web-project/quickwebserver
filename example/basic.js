import QuickWebServer from '../src/QuickWebServer.js';

const server = new QuickWebServer();

server.get('/', async (data, response) => {
  response.type('text/html');
  response.send('Hello from QuickJS!');
});

server.get('/post', (data, response) => {
  response.type('text/html');
  response.send('POST: Hello from QuickJS!');
});

server.get('/test', (data, response) => {
  response.type('application/json');
  response.send({
    apiActive: true,
    users: [
      {
        id: 0,
        nickname: 'LyohaPlotinka',
        status: 'active'
      }
    ]
  });
});

server.listen(3000);
