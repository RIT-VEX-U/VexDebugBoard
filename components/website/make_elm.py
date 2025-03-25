import subprocess
import gzip
import sys
import platform
mainPath = sys.argv[1]
jsPath = sys.argv[2]
minJsPath = sys.argv[3]
gzPath = sys.argv[4]
optimFlag = sys.argv[5]
subprocess.run(["elm", "make", f"{mainPath}", f"{optimFlag}", f"--output={jsPath}"], check=True)
if('windows' in platform.system().lower()):
    uglifyPath = "uglifyjs.cmd"
else:
    uglifyPath = "uglifyjs"
subprocess.run([uglifyPath, f"{jsPath}", f"--compress", "pure_funcs=[F2,F3,F4,F5,F6,F7,F8,F9,A2,A3,A4,A5,A6,A7,A8,A9],pure_getters,keep_fargs=false,unsafe_comps,unsafe", "-o", f"{jsPath}.part"], check=True)
subprocess.run([uglifyPath, f"{jsPath}.part","--mangle", f"--output", f"{minJsPath}"], check=True)

with gzip.open(gzPath, 'wb') as f:
    with open(minJsPath, 'rb') as f2:
        f.write(f2.read())

# subprocess.run("gzip", f"-k --best -c {minJsPath} > {gzPath}")