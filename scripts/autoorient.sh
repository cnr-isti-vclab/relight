#!/bin/bash
for f in *.JPG *.jpg; do convert $f -auto-orient $f; done
