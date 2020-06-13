import emscripten from './build/module.js'
import template from './index.html';

addEventListener('fetch', event => {
    event.respondWith(handleRequest(event))
})

let emscripten_module = new Promise((resolve, reject) => {
    emscripten({
        instantiateWasm(info, receive) {
            let instance = new WebAssembly.Instance(wasm, info)
            receive(instance)
            return instance.exports
        },
    }).then(module => {
        resolve({
            init: module.cwrap('init', 'number', ['number']),
            generate_string: module.cwrap('generate_string', 'number', ['string', 'string', 'string', 'string', 'string', 'string', 'string']),
            structure_info_string: module.cwrap('structure_info_string', 'string', ['string', 'string', 'string', 'string', 'string']),
            module: module,
        })
    })
})

const list_features = ['village', 'desert_pyramid', 'jungle_pyramid', 'shipwreck', 'mansion', 'ruined_portal', 'outpost', 'swamp_hut', 'igloo', 'ocean_ruin', 'treasure', 'spawn'];
const sizeX = 128;
const sizeZ = 128;

const indexPage = 'use /features.json or /map.png';
const errorReferer = 'this page is protected against hotlinking';
const publishAddress = PUBLISH_ADDRESS;
const defaultSeed = DEFAULT_SEED;

function checkReferer(request) {
    if (request.headers.get('referer') == null || request.headers.get('referer') == '' || request.headers.get('referer').indexOf(publishAddress) == 0) {
        return true;
    }
    return false;
}

async function handleRequest(event) {
    let cache = caches.default;
    let values = ["0", "0", "0", "512", "512", "1", "1.16"]

    const listvars = ['seed', 'x', 'z', 'version', 'scale'];
    let uservalues = [DEFAULT_SEED, "0", "0", "1.16", "1"];

    let request = event.request;
    let url = new URL(request.url);

    let hasSeed = (url.searchParams.get('seed') != undefined);

    for (let i = 0; i < listvars.length; i++) {
        let prev = uservalues[i];
        uservalues[i] = url.searchParams.get(listvars[i]);
        if (!uservalues[i]) uservalues[i] = prev;
    }

    values[0] = uservalues[0]
    values[1] = (parseInt(uservalues[1]) * sizeX).toString();
    values[2] = (parseInt(uservalues[2]) * sizeZ).toString();
    values[3] = sizeX.toString();
    values[4] = sizeZ.toString();
    if (uservalues[4] == "2" || uservalues[4] == "1") {
        values[5] = uservalues[4];
    }
    values[6] = uservalues[3];

    let biomes_module = await emscripten_module

    let response = await cache.match(request)
    if (response) {
        return response;
    }

    if (url.pathname == '/features.json') {
        if (!checkReferer(event.request)) {
            return new Response(errorReferer);
        }

        let features = []

        for (let i in list_features) {
            let feature = biomes_module.structure_info_string(values[0], uservalues[1], uservalues[2], list_features[i], values[6]);
            let fp = JSON.parse(feature);
            if (fp.viable == 1) {
                delete fp.viable;
                features.push(fp);
            }
        }
        let response = new Response(JSON.stringify(features), {
            headers: {
                "Content-Type": "application/json",
                "Cache-Control": "max-age=3600",
            }
        });
        await cache.put(request, response.clone());
        return response;

    } else if (url.pathname == '/map.png') {
        if (!checkReferer(event.request)) {
            return new Response(errorReferer);
        }

        let bytes = new Uint8Array(1000000);
        let ptr = biomes_module.init(bytes.length);
        biomes_module.module.HEAPU8.set(bytes, ptr)
        let newSize = biomes_module.generate_string(values[0], values[1], values[2], values[3], values[4], values[5], values[6]);
        if (newSize == 0) {
            return new Response(bytes, 'none')
        }
        let resultBytes = biomes_module.module.HEAPU8.slice(ptr, ptr + newSize)
        let response = new Response(resultBytes, {
            headers: {
                "Content-Type": "image",
                "Cache-Control": "max-age=3600",
            }
        });
        await cache.put(request, response.clone());
        return response;

    } else {
        let templateModified = template.replace(/{OGIMAGETEMPLATE}/i, '<meta property="og:image" content="' + publishAddress + '/map.png?scale=2&seed=' + values[0] + '&x=' + (uservalues[1] / sizeX) + '&z=' + (uservalues[2] / sizeZ) + '" />');
        templateModified = templateModified.replace(/{INPUTTEMPLATE}/i, '<input type="text" id="seed" value="' + values[0] + '" /></label><label>X: <input type="text" id="longitude" value="' + uservalues[1] + '" /></label><label>Z: <input type="text" id="latitude" value="' + uservalues[2] + '" /></label>');
        let response = new Response(templateModified, {
            headers: {
                "Content-Type": "text/html",
                "Cache-Control": "max-age=3600",
            }
        });
        await cache.put(request, response.clone());
        return response;
    }

}