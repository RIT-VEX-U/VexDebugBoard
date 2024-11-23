# Makefile to make the elm website, minify and compress it, then build it into a c file to be flashed to the chip
index_html_c = "c_src/index_html.c"

# Extra lines of c to make the encoded data accessible
include_header = "#include \"website.h\""
define elm_js_funcs
char *get_elm_min_js() { return (char *)build_elm_min_js; }
unsigned long get_elm_min_js_size() { return build_elm_min_js_len; }
endef
define index_html_funcs
char *get_index_html() { return (char *)build_index_html; }
unsigned long get_index_html_size() { return build_index_html_len; }
endef

website:
	elm make --optimize src/Main.elm --output=build/elm.js
	uglifyjs build/elm.js --compress 'pure_funcs=[F2,F3,F4,F5,F6,F7,F8,F9,A2,A3,A4,A5,A6,A7,A8,A9],pure_getters,keep_fargs=false,unsafe_comps,unsafe' | uglifyjs --mangle --output build/elm.min.js

	echo ${include_header} > ${index_html_c}
	xxd -i build/index.html >> ${index_html_c}
	echo ${index_html_funcs} >> ${index_html_c}

# xxd -i build/elm.min.js > build/elm_min_js.c
	