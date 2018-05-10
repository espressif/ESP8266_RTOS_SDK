#!/bin/bash

#
# Generate the certificates and keys for testing.
#

SAVEIFS=$IFS
IFS=$(echo -en "\n\b")

ROOT_SUBJECT="/C=C1/ST=JS1/L=WX1/O=ESP1/OU=ESP1/CN=Server1 CA/emailAddress=ESP1"
LEVEL2_SUBJECT="/C=C2/ST=JS22/L=WX22/O=ESP22/OU=ESP22/CN=Server22 CA/emailAddress=ESP22"
LEVEL3_SUBJECT="/C=C3/ST=JS333/L=WX333/O=ESP333/OU=ESP333/CN=Server333 CA/emailAddress=ESP333"

# private key generation
openssl genrsa -out ca.key 2048
openssl genrsa -out server.key 2048
openssl genrsa -out client.key 2048

# cert requests
openssl req -new -key ca.key -out ca.csr -text -subj $ROOT_SUBJECT
openssl req -new -key server.key -out server.csr -text -subj $LEVEL2_SUBJECT
openssl req -new -key client.key -out client.csr -text -subj $LEVEL3_SUBJECT

# generate the actual certs.
openssl x509 -req -in ca.csr -out ca.pem -sha256 -days 5000 -signkey ca.key -text -extensions v3_ca
openssl x509 -req -in server.csr -out server.pem -sha256 -CAcreateserial -days 5000 -CA ca.pem -CAkey ca.key -text -extensions v3_ca
openssl x509 -req -in client.csr -out client.pem -sha256 -CAcreateserial -days 5000 -CA ca.pem -CAkey ca.key -text -extensions v3_ca

rm *.csr
rm *.srl

mv ca.* ./main
mv server.* ./main
mv client.* ./main
