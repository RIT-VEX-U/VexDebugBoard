set -e
# $1 is path to main.elm
# $2 is path for .js file
# $3 is path for .min.js file
# $4 is path for .gz file

# $5 is optimization flag --debug or --optimize
elm make $1 $5 --output=$2
uglifyjs $2 --compress 'pure_funcs=[F2,F3,F4,F5,F6,F7,F8,F9,A2,A3,A4,A5,A6,A7,A8,A9],pure_getters,keep_fargs=false,unsafe_comps,unsafe' | uglifyjs --mangle --output $3
gzip -k --best -c $3 > $4