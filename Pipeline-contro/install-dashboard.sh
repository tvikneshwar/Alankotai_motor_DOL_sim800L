#!/bin/bash
# Run on the Raspberry Pi from the folder that contains index.html:
#   bash install-dashboard.sh
set -e
WEBROOT=/var/www/scada

sudo apt update
sudo apt install -y nginx curl

sudo mkdir -p $WEBROOT/vendor
sudo cp index.html $WEBROOT/index.html

# Libraries are stored locally so the dashboard works on an isolated network.
# (Needs internet once.)
sudo curl -fL -o $WEBROOT/vendor/mqtt.min.js https://cdn.jsdelivr.net/npm/mqtt@5.10.1/dist/mqtt.min.js

# Tailwind CSS is compiled locally (no dependency on cdn.tailwindcss.com). Needs Node/npm.
echo "Building Tailwind CSS..."
BUILD=$(mktemp -d)
cp index.html "$BUILD/"
(
  cd "$BUILD"
  npm init -y >/dev/null
  npm install --silent tailwindcss@3.4.17
  echo "module.exports = { content: ['./index.html'] }" > tailwind.config.js
  printf '@tailwind base;\n@tailwind components;\n@tailwind utilities;\n' > in.css
  npx tailwindcss -c tailwind.config.js -i in.css -o out.css --minify
)
sudo cp "$BUILD/out.css" $WEBROOT/vendor/tailwind.css
rm -rf "$BUILD"

sudo tee /etc/nginx/sites-available/scada >/dev/null <<'CONF'
server {
    listen 80 default_server;
    listen [::]:80 default_server;
    root /var/www/scada;
    index index.html;
    location / {
        try_files $uri $uri/ =404;
        add_header Cache-Control "no-cache";
    }
}
CONF

sudo ln -sf /etc/nginx/sites-available/scada /etc/nginx/sites-enabled/scada
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t
sudo systemctl enable --now nginx
sudo systemctl reload nginx

echo
echo "Done. Open  http://$(hostname -I | awk '{print $1}')/  from any device on the network."
