from gevent.pywsgi import WSGIServer
from app import app
from gevent import monkey

monkey.patch_all()

http_server = WSGIServer(('', 5000), app)
http_server.serve_forever()
