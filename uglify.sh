#!/bin/bash

uglifyjs relight.js > relight.min.js
cat relight.min.js >> relight-viewer.min.js
uglify relight-viewer.js >> relight-viewer.min.js
