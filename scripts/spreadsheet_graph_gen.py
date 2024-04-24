import sys
import csv
import os

file = sys.argv[1]
sbdir = sys.argv[2]
m = {}

with open(file) as f:
    csv_reader = csv.reader(f)
    csv_header = next(csv_reader)
    for row in csv_reader:
        m[(row[0], row[1])] = row[2:]

benches = [i.split('.')[0].split('_')[0] for i in os.listdir(f'trace/{sbdir}/')]
schemes = ['lru', 'hawkeye', 'perceptron', 'ehc', 'both']

print("bench", end=',')
for scheme in schemes:
    print(f'{scheme}', end='')
    if (scheme != 'both'):
        print(',', end='')

print()

for bench in benches:
    print(bench, end=',')
    for scheme in schemes:
        if (not (scheme, bench) in m):
            print('', end='')
        else:
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
