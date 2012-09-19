#!/usr/bin/python3

import sys, signal, subprocess

class InterruptedError(Exception):
    pass

def exceptionRaiser(signum, frame):
    raise(InterruptedError(signum))

if __name__=='__main__':
    signal.signal(signal.SIGTERM, exceptionRaiser)
    signal.signal(signal.SIGINT, exceptionRaiser)
    while True:
        with subprocess.Popen(sys.argv[1:], stdout=sys.stdout, stderr=sys.stderr) as proc:
            ret = None
            try:
                ret = proc.wait()
            finally:
                if ret==None:
                    proc.terminate()
                elif ret==0:
                    break

