"""Local SIP interoperability tests for the ingress reliable transport selection fix."""
from contextlib import ExitStack
import hashlib
import json
import os
from pathlib import Path
import queue
import re
import socket
import ssl
import subprocess
import tempfile
import threading
import time

ROOT = Path(os.environ["MYTEL_SIP_TEST_WORK"]).resolve()
INSTALL = ROOT / "install"
LOOP = "127.0.0.1"

def endpoint(stack, udp=False):
    s=stack.enter_context(socket.socket(type=socket.SOCK_DGRAM if udp else socket.SOCK_STREAM))
    s.bind((LOOP,0))
    s.settimeout(3)
    if not udp: s.listen()
    return s

def port(s): return s.getsockname()[1]

def wire(method,user,transport,via_port,tag, rport=False):
    return (f"{method} sip:{user}@127.0.0.1 SIP/2.0\r\n"
        f"Via: SIP/2.0/{transport} 127.0.0.1:{via_port};branch=z9hG4bK-{tag}"+(";rport" if rport else "")+"\r\n"
        f"From: <sip:caller@localhost>;tag=f-{tag}\r\nTo: <sip:{user}@localhost>\r\n"
        f"Call-ID: {tag}@localhost\r\nCSeq: 1 {method}\r\nMax-Forwards: 70\r\n"
        "Contact: <sip:caller@127.0.0.1>\r\nContent-Length: 0\r\n\r\n").encode("ascii")

class Conn:
    def __init__(self, sock):
        self.sock=sock; self.buf=b""
    def read(self):
        while b"\r\n\r\n" not in self.buf:
            block=self.sock.recv(8192)
            if not block: raise RuntimeError("unexpected EOF")
            self.buf+=block
        head,self.buf=self.buf.split(b"\r\n\r\n",1)
        return head
    def responses(self, tag, expected):
        got=[]
        for _ in expected:
            head=self.read()
            assert b"Call-ID: "+tag.encode()+b"@localhost" in head, "response arrived on wrong socket"
            got.append(int(head.split(b" ",2)[1]))
        assert got==expected,(got,expected)

def launch(stack, variant, transport, wire_port, ca):
    lib_colon=str(INSTALL/"lib")
    if variant=="baseline": lib_colon=str(ROOT/"red")+":"+lib_colon
    env=os.environ|{"LD_LIBRARY_PATH":lib_colon}
    proc=subprocess.Popen([str(ROOT/"return-test"),transport,str(wire_port),str(ca)],stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,env=env)
    def stop():
        if proc.poll() is None: proc.terminate()
        proc.wait(3)
    stack.callback(stop)
    ready=queue.Queue()
    def drain():
        for line in proc.stdout:
            seen=re.fullmatch(rb"READY (\d+)\s*",line)
            if seen: ready.put(int(seen[1]))
    threading.Thread(target=drain,daemon=True).start()
    return ready.get(timeout=4)

def tls_or_tcp(stack,server,context):
    raw,_=server.accept()
    s=stack.enter_context(raw);s.settimeout(3)
    if context: s=stack.enter_context(context.wrap_socket(s,server_side=True))
    return Conn(s)

def steady(variant, transport, ca, context):
    with ExitStack() as stack:
        server=endpoint(stack);trap=endpoint(stack)
        launch(stack,variant,transport,port(server),ca)
        sockets=[tls_or_tcp(stack,server,context) for _ in range(2)]
        for c in sockets:
            # Each outgoing OPTIONS uses a separate bank, same endpoint. Alive without retransmissions.
            initial=c.read()
            assert initial.startswith(b"OPTIONS "), "client stream setup failed"
        if variant=="baseline":
            sockets[0].sock.sendall(wire("INVITE","accept",transport,port(trap),"red"))
            other=tls_or_tcp(stack,trap,context)
            other.responses("red",[100,180,200])
            return "misrouted_100_180_200_to_Via_port"
        for i,(c, tag, final) in enumerate(zip(sockets,("accept","decline"),(200,486))):
            c.sock.sendall(wire("INVITE",tag,transport,port(trap),tag))
            c.responses(tag,[100,180,final])
            c.sock.sendall(wire("OPTIONS","now",transport,port(trap),"opt"+str(i)))
            c.responses("opt"+str(i),[200])
        trap.settimeout(.15)
        try:s,_=trap.accept()
        except TimeoutError:pass
        else:s.close(); raise AssertionError("unexpected new connection to Via port")
        trap.settimeout(3)
        sockets[0].sock.sendall(wire("OPTIONS","later",transport,port(trap),"after-close"))
        sockets[0].sock.shutdown(socket.SHUT_RDWR);sockets[0].sock.close()
        other=tls_or_tcp(stack,trap,context)
        other.responses("after-close",[200])
        # Stateful final 486 must use the Via fallback after its 100/180 flow closes.
        sockets[1].sock.sendall(wire("INVITE","later-invite",transport,port(trap),"closed-dialog"))
        sockets[1].responses("closed-dialog",[100,180])
        sockets[1].sock.shutdown(socket.SHUT_RDWR);sockets[1].sock.close()
        # The closed stateless transaction created a reusable connection in the same bank as sockets[0].
        # The other bank independently opens a second fallback connection.
        next_conn=tls_or_tcp(stack,trap,context)
        next_conn.responses("closed-dialog",[486])
        return "two_scoped_flows_stateless_and_stateful_closed_fallback_pass"

def udp(variant, ca):
    with ExitStack() as stack:
        sender=endpoint(stack,True);other=endpoint(stack,True)
        target=launch(stack,variant,"UDP",port(other),ca)
        for rport in (False,True):
            tag="udp"+str(int(rport))
            sender.sendto(wire("INVITE","decline","UDP",port(other),tag,rport), (LOOP,target))
            receiving=sender if rport else other
            expected=[100,180,486]
            for status in expected:
                message,_=receiving.recvfrom(4096)
                assert int(message.split(b" ",2)[1])==status and (b"Call-ID: "+tag.encode()+b"@localhost") in message
    return "via_and_symmetric_rport_pass"

with tempfile.TemporaryDirectory() as temporary:
    private=Path(temporary);cert=private/"ca.pem";key=private/"server.key"
    subprocess.run(["openssl","req","-x509","-nodes","-newkey","rsa:2048","-days","1","-subj","/CN=127.0.0.1","-addext","subjectAltName=IP:127.0.0.1","-addext","basicConstraints=critical,CA:TRUE","-out",str(cert),"-keyout",str(key)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.maximum_version=ssl.TLSVersion.TLSv1_2;ctx.load_cert_chain(cert,key)
    for ver in ("baseline","patched"):
        for protocol in ("TCP","TLS","UDP"):
            result=udp(ver,cert) if protocol=="UDP" else steady(ver,protocol,cert,ctx if protocol=="TLS" else None)
            print(json.dumps({"variant":ver,"protocol":protocol,"result":result}),flush=True)
