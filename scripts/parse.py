import re
import sys

file = sys.argv[1]
scheme = sys.argv[2]

result = set()

with open(file) as f:
    lines = f.readlines()
    
    new = True
    cnl = 0
    header=""
    ipc=""
    total=""
    hit=""
    miss=""
    for line in lines:
        if new:
            header = line.split('.')[0].split('_')[0]
            new = False
            ipc=""
            total=""
            hit=""
            miss=""        
            # print (header)
        else:
            if line == '\n':
                cnl += 1
            else:
                res = re.search("CPU 0 cummulative IPC", line)
                if res:
                    ipc = line.split(':')[1][:-len("instructions")].lstrip()
                    # print(ipc)
                
                res = re.search("LLC TOTAL", line)
                if res:
                    tmp = line.split(':')
                    total = tmp[1].lstrip().split(' ')[0].lstrip()
                    hit = tmp[2].lstrip().split(' ')[0].lstrip()
                    miss = tmp[3].lstrip().split(' ')[0].strip('\n')
                    # print(total)
                    # print(hit)
                    # print(miss)
            
            if cnl == 2:
                new = True
                cnl = 0
                if header and not ipc:
                  result.add(scheme + "," + header + ",,,,")
                header=""

        if header and ipc and total and hit and miss:
            result.add(scheme + "," + header + "," + total + "," + hit + "," + miss + "," + ipc)

for r in sorted(result):
    print(r)