# Genreated source files
set(index_html_c ${CMAKE_CURRENT_BINARY_DIR}/index_html.c)
set(elm_js_c ${CMAKE_CURRENT_BINARY_DIR}/elm_js.c)
idf_component_register(SRCS ${index_html_c} ${elm_js_c} INCLUDE_DIRS "include")

# Actual Sources
set(index_html ${CMAKE_CURRENT_SOURCE_DIR}/src/index.html)
set(ELM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE ELM_SOURCES ${ELM_SRC_DIR}/*.elm)

# Generated Resources
set(elm_js ${CMAKE_CURRENT_BINARY_DIR}/elm.js)
set(elm_min_js ${CMAKE_CURRENT_BINARY_DIR}/elm.min.js)
set(elm_min_js_gz ${CMAKE_CURRENT_BINARY_DIR}/elm.min.js.gz)


set(ELM_OPT_FLAG --debug)


# Embed index.html into binary
add_custom_command(OUTPUT ${index_html_c}
COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/embed_file.py "index_html" ${index_html} ${index_html_c}
DEPENDS ${index_html} ${CMAKE_CURRENT_SOURCE_DIR}/embed_file.py)

# Embed elm.js into binary
add_custom_command(OUTPUT ${elm_js_c} 
COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/embed_file.py "elm_min_js" ${elm_min_js_gz} ${elm_js_c}
DEPENDS ${elm_min_js_gz} ${elm_min_js} ${elm_js} ${CMAKE_CURRENT_SOURCE_DIR}/embed_file.py)

# Build elm application into .js, .min.js, and .min.js.gz file
add_custom_command(OUTPUT ${elm_js} ${elm_min_js} ${elm_min_js_gz}
COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/make_elm.py ${ELM_SRC_DIR}/Main.elm ${elm_js} ${elm_min_js} ${elm_min_js_gz} ${ELM_OPT_FLAG}
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
DEPENDS ${ELM_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/make_elm.py )


# set(UGLIFY_CMD "uglifyjs ${ELM_JS_OUTPUT} --compress 'pure_funcs=[F2,F3,F4,F5,F6,F7,F8,F9,A2,A3,A4,A5,A6,A7,A8,A9],pure_getters,keep_fargs=false,unsafe_comps,unsafe' | uglifyjs --mangle --output ${ELM_MIN_JS_OUTPUT}")


# add_custom_command(OUTPUT ${ELM_JS_OUTPUT} COMMAND elm make ${ELM_MAIN} ${ELM_OPT_FLAG} --output=${ELM_JS_OUTPUT})
# add_custom_command(OUTPUT ${ELM_MIN_JS_OUTPUT} COMMAND ${UGLIFY_CMD})

# add_custom_command(OUTPUT c_srcs/index_html.c COMMAND echo "#include \"website.h\"' >> c_srcs/index.html COMMAND xxd -n "index_html" -i src/index.html >>  CDEPENDS src/index.html)
# add_custom_command(OUTPUT c_srcs/elm_min_js.c COMMAND xxd -n "elm_min_js" -i build/index.html DEPENDS src/index.html)


# char *get_elm_min_js() { return (char *)build_elm_min_js; }
# unsigned long get_elm_min_js_size() { return build_elm_min_js_len; }


# char *get_index_html() { return (char *)build_index_html; }
# unsigned long get_index_html_size() { return build_index_html_len; }