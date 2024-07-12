website:
	elm make src/Main.elm --output=build/elm.js
	uglifyjs build/elm.js --compress 'pure_funcs=[F2,F3,F4,F5,F6,F7,F8,F9,A2,A3,A4,A5,A6,A7,A8,A9],pure_getters,keep_fargs=false,unsafe_comps,unsafe' | uglifyjs --mangle --output build/elm.min.js

	xxd -i build/index.html > build/index_html.c
	xxd -i build/elm.min.js > build/elm_min_js.c
