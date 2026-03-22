import os
import time

seed = 47
startTime = 60
stopTime = 120

CMD = './tools/sim "scene --RngRun=47 --startTime=60 --stopTime=120 --mode={mode}{enableHello}{disableRsu}" "out/{outFile}"'

def prepare(mode, enableHello, disableRsu, outFile):
    enableHello = " --enableHello" if enableHello else ''
    disableRsu = " --disableRsu" if disableRsu else ''

    return CMD.format(mode=mode, enableHello=enableHello, disableRsu=disableRsu, outFile=outFile)

cases = [
    ['low', False, False, 'v2v-low.log'],
    ['medium', False, False, 'v2v-medium.log'],
    ['high', False, False, 'v2v-high.log'],
    ['low', True, False, 'v2i-low.log'],
    ['medium', True, False, 'v2i-medium.log'],
    ['high', True, False, 'v2i-high.log'],
    ['low', True, True, 'hybrid-low.log'],
    ['medium', True, True, 'hybrid-medium.log'],
    ['high', True, True, 'hybrid-high.log'],
]

for case in cases:
    cmd = prepare(*case)
    print('-' * 50)
    print("Execute:", cmd)
    print('-' * 50)

    start_time = time.perf_counter()
    if os.system(cmd) != 0:
        break
    end_time = time.perf_counter()
    elapsed_time = end_time - start_time

    print('-' * 50)
    print(f"Took: {elapsed_time:.4f} seconds")
    print('-' * 50)
