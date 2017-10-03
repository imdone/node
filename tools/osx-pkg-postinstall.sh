#!/bin/sh
# TODO Can this be done inside the .pmdoc? id:3804
# TODO Can we extract $PREFIX from the installer? id:4124
cd /usr/local/bin
ln -sf ../lib/node_modules/npm/bin/npm-cli.js npm
ln -sf ../lib/node_modules/npm/bin/npx-cli.js npx
