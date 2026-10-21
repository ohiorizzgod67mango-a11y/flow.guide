from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import ctypes as C
import subprocess, json, math, argparse, hashlib
ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parent if (ROOT.parent / 'src/engine').exists() else ROOT.parent / 'flow-assist'
ENGINE_HASH = hashlib.sha256((PROJECT/'src/engine/TimingAssist.cpp').read_bytes() + (PROJECT/'src/engine/TimingAssist.hpp').read_bytes()).hexdigest()[:12]
SO = ROOT / 'engine-native.so'
subprocess.run(['g++', '-std=c++23', '-O2', '-shared', '-fPIC', '-I'+str(PROJECT/'src'), str(ROOT/'bridge.cpp'), str(PROJECT/'src/engine/TimingAssist.cpp'), '-o', str(SO)], check=True)
subprocess.run(['g++', '-std=c++23', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(PROJECT/'src'), str(PROJECT/'tests/TimingAssistTest.cpp'), str(PROJECT/'src/engine/TimingAssist.cpp'), '-o', str(ROOT/'engine-tests')], check=True)
CHECKS = subprocess.run([str(ROOT/'engine-tests')], capture_output=True, text=True, check=True).stdout.strip()
class Event(C.Structure):
    _fields_=[('seconds',C.c_double),('button',C.c_int),('player2',C.c_int),('down',C.c_int)]
class Metrics(C.Structure):
    _fields_=[('corrected',C.c_int),('pending',C.c_int),('emitted',C.c_int),('physical',C.c_double),('target',C.c_double),('scheduled',C.c_double),('delivered',C.c_double),('lateness',C.c_double)]
lib=C.CDLL(str(SO))
lib.simulate.argtypes=[C.c_double,C.c_double,C.c_int,C.c_int,C.c_double,C.POINTER(C.c_double),C.c_int,C.POINTER(Event),C.c_int,C.POINTER(Event),C.c_int,C.POINTER(Metrics)]
lib.simulate.restype=C.c_int

def number(x, low, high):
    value=float(x)
    if not math.isfinite(value) or not low <= value <= high: raise ValueError('value outside supported range')
    return value

def simulate(data):
    if not isinstance(data,dict): raise ValueError('expected a json object')
    rate=number(data.get('rate',240),30,2000)
    window=number(data.get('window',3),0,6)
    duration=number(data.get('duration',0),0,20)
    simrate=int(number(data.get('simrate',240),30,1000))
    targets=data.get('targets',[])
    if not isinstance(targets,list) or len(targets)>20000: raise ValueError('too many targets')
    targets=[number(t,0,1000000) for t in targets]
    physical=data.get('events',[])
    if not isinstance(physical,list) or len(physical)>2048: raise ValueError('too many physical events')
    events=[]
    for e in physical:
        events.append(Event(number(e['time'],0,20),1,0,bool(e['down'])))
    events.sort(key=lambda e:e.seconds)
    arr=(Event*len(events))(*events)
    out=(Event*8192)();metrics=Metrics()
    ts=(C.c_double*len(targets))(*targets)
    n=lib.simulate(rate,window,int(bool(data.get('enabled',True))),simrate,duration,ts,len(targets),arr,len(events),out,8192,C.byref(metrics))
    return {'events':[{'time':out[i].seconds,'down':bool(out[i].down)} for i in range(n)],
            'metrics':{name:getattr(metrics,name) for name,_ in metrics._fields_},
            'engine':'shared c++ source', 'engineHash':ENGINE_HASH, 'simrate':simrate}

class Handler(BaseHTTPRequestHandler):
    def reply(self,status,payload,kind='application/json'):
        raw=payload if isinstance(payload,bytes) else json.dumps(payload).encode()
        self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(raw)));self.send_header('Cache-Control','no-store');self.end_headers();self.wfile.write(raw)
    def do_GET(self):
        if self.path in ('/','/index.html'): self.reply(200,(ROOT/'index.html').read_bytes(),'text/html; charset=utf-8')
        elif self.path=='/api/checks': self.reply(200,{'passed':True,'engineHash':ENGINE_HASH,'summary':CHECKS,'limitations':'simplified physics, not geometry dash or a gd compatibility test'})
        else:self.reply(404,{'error':'not found'})
    def do_POST(self):
        if self.path!='/api/simulate': return self.reply(404,{'error':'not found'})
        try:
            length=int(self.headers.get('Content-Length','0'))
            if not 0 < length <= 300000: raise ValueError('request too large or empty')
            data=json.loads(self.rfile.read(length));result=simulate(data);self.reply(200,result)
        except (ValueError,KeyError,TypeError) as error:self.reply(400,{'error':str(error)})
    def log_message(self,*args):pass

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=8080);args=parser.parse_args()
    print('native engine loaded | '+CHECKS,flush=True)
    ThreadingHTTPServer(('0.0.0.0',args.port),Handler).serve_forever()
