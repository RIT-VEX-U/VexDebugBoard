import sys

cname = sys.argv[1]
inputFile = sys.argv[2]
outputFile = sys.argv[3]

with open(inputFile, 'rb') as f:
    b = f.read()
    l = [str(num) for num in b]

    template = f"""
    const unsigned char {cname}[] = {{{','.join(l)}}};
    const unsinged int {cname}_length = {len(l)};
    """
    print(template)

with open(outputFile, 'w') as f:
    f.write(template)