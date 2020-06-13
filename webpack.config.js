const webpack = require('webpack');
const CopyPlugin = require('copy-webpack-plugin');
const path = require('path');
const spawn = require('child_process').spawnSync;
const Handlebars = require('handlebars');

const PUBLISH_ADDRESS = 'https://biomes.lspgn.workers.dev';
let tmpRnd = Math.random().toString();
if (tmpRnd.length >= 2) {
    tmpRnd = tmpRnd.slice(2);
}
const SEED = tmpRnd

module.exports = {
    context: path.resolve(__dirname, '.'),
    devtool: 'nosources-source-map',
    entry: './index.js',
    target: 'webworker',
    optimization: {
        minimize: false
    },
    plugins: [{
            apply: compiler => {
                compiler.hooks.compilation.tap('emscripten-build', compilation => {
                    let result = spawn('node', ['build.js'], {
                        stdio: 'inherit'
                    })

                    if (result.status != 0) {
                        compilation.errors.push('emscripten build failed')
                    } else {
                        console.log('emscripten build complete')
                    }
                })
            },
        },
        new CopyPlugin([{
            from: './build/module.wasm',
            to: './worker/module.wasm'
        }, ]),
        //new webpack.EnvironmentPlugin(['PUBLISH_ADDRESS'])
        new webpack.DefinePlugin({
            PUBLISH_ADDRESS: JSON.stringify(PUBLISH_ADDRESS),
            DEFAULT_SEED: JSON.stringify(SEED),
        })
    ],
    module: {
        rules: [{
                test: /emscripten\.js$/,
                loader: 'exports-loader',
            },
            {
                test: /index\.html$/,
                loader: 'html-loader',
                options: {
                    preprocessor: (content, loaderContext) => {
                        let result;
                        try {
                            result = Handlebars.compile(content)({
                                publish_address: PUBLISH_ADDRESS,
                            });
                        } catch (error) {
                            loaderContext.emitError(error);
                            return content;
                        }
                        return result;
                    },
                },
            },
        ],
    },
}