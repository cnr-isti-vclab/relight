#!/bin/bash

uglifyjs relight-core.js > relight.min.js
uglifyjs relight-shaders.js >> relight.min.js
uglifyjs relight-canvas.js >> relight.min.js
cat relight.min.js > relight-viewer.min.js
uglifyjs relight-interface.js >> relight-viewer.min.js
uglifyjs hammer.js > hammer.min.js

