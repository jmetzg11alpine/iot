#!/bin/bash

# Create a directory for certificates
mkdir -p certs
cd certs

# Generate a private key for the CA
openssl genrsa -out ca.key 2048

# Create the root certificate (CA)
openssl req -x509 -new -nodes -key ca.key -sha256 -days 365 -out ca.crt -subj "/CN=MyIoTCA"

# Generate a private key for the server
openssl genrsa -out server.key 2048

# Create a Certificate Signing Request (CSR) for the server
openssl req -new -key server.key -out server.csr -subj "/CN=iot-white-pond-1937.fly.dev"

# Generate the server certificate with SANs for both local and production
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days 365 -sha256 \
  -extfile <(printf "subjectAltName=DNS:iot-white-pond-1937.fly.dev,DNS:localhost,IP:127.0.0.1,IP:192.168.1.252")

# Generate the ca.pem, need to move the ca_pem.h into the esp-idf project
openssl x509 -in ca.crt -out ca.pem -outform PEM
xxd -i ca.pem > ca_pem.h

# Output the generated files
echo "Generated the following files in the 'certs' directory:"
ls -l
