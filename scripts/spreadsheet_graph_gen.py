import sys
import csv

file = sys.argv[1]
m = {}

# read csv:
with open(file) as f:
    csv_reader = csv.reader(f)
    csv_header = next(csv_reader)
    for row in csv_reader:
        m[(row[0], row[1])] = row[2:]

for bench in ['GemsFDTD','astar','bwaves','bzip2','cactusADM','calculix','gamess','gcc','gobmk','gromacs','h264ref','sphinx3','zeusmp']:
    for scheme in ['hawkeye', 'perceptron', 'ehc', 'both']:
        total = m[(scheme, bench)][0]
        hit = m[(scheme, bench)][1]
        miss = m[(scheme, bench)][2]
        ipc = m[(scheme, bench)][3]
        if total:
            print(int(hit) / int(total), end='')
        else:
            print('', end='')
        if (scheme != 'both'):
          print(',', end='')
    
    print()
