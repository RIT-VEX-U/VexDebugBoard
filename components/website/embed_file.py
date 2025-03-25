import sys

cname = sys.argv[1]
inputFile = sys.argv[2]
outputFile = sys.argv[3]

with open(inputFile, 'rb') as f:
    b = f.read()
    l = [str(num) for num in b]

    template = f"""
    static const char {cname}[] = {{{','.join(l)}}};
    static const unsigned int {cname}_length = {len(l)};
    const char* get_{cname}(){{return {cname};}}
    unsigned int get_{cname}_size(){{return {cname}_length;}}
    """

with open(outputFile, 'w') as f:
    f.write(template)