#!/bin/bash

if [ -f .env ]; then
    export $(grep -v '^#' .env | xargs)
fi


read -sp "Enter password for $PI_USER@$PI_HOST: " PASSWORD
echo

# sshpass -p "$PASSWORD" scp -r docker-compose.yml frontend/Dockerfile.frontend frontend/build frontend/nginx.conf backend/backend backend/Dockerfile.backend $PI_USER@$PI_HOST:$PI_PROJECT_DIR || { echo "Failed to transfer files"; exit 1; }

sshpass -p "$PASSWORD" scp -r docker-compose.yml f backend/backend backend/Dockerfile.backend $PI_USER@$PI_HOST:$PI_PROJECT_DIR || { echo "Failed to transfer files"; exit 1; }

sshpass -p "$PASSWORD" ssh $PI_USER@$PI_HOST << EOF
    cd Desktop/iot
    docker-compose down
    docker-compose build --no-cache
    docker-compose up
    exit
EOF


echo "Deployment to finished"
