#!/bin/bash

uglifyjs relight.js > relight.min.js
cat relight.min.js > relight-viewer.min.js
uglifyjs relight-viewer.js >> relight-viewer.min.js
