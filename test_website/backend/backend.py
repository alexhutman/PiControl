import socketio

from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler

sio = socketio.Server(async_mode='gevent', logger=True, engineio_logger=True)
app = socketio.WSGIApp(sio)
pywsgi.WSGIServer(('', 8000), app,
        handler_class=WebSocketHandler).serve_forever()

@sio.event
def connect(sid, environ, auth):
    print("Connected poggers")

if __name__ == "__main__":
    app.run()
