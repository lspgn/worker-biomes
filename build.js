var $ = require('shelljs')

if (!$.which('docker')) {
  $.echo(
    'This template requires Docker. Please install Docker and try again.',
  )
  $.exit(1)
}

$.mkdir('-p', 'build')
$.exec(
  'docker run --rm -v $(pwd):/src trzeci/emscripten:sdk-tag-1.38.32-64bit emcc -O2 -s WASM=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=\'["cwrap", "setValue"]\' -s ALLOW_MEMORY_GROWTH=1 -s DYNAMIC_EXECUTION=0 -s TEXTDECODER=0 -s MODULARIZE=1 -s ENVIRONMENT=\'web\' -s EXPORT_NAME="emscripten" --pre-js \'./pre.js\' -o ./build/module.js -o ./build/module.js ./src/main.c ./src/cubiomes/finders.c ./src/cubiomes/generator.c ./src/cubiomes/layers.c ./src/cubiomes/util.c',
)
