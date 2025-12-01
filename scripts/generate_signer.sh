#!/usr/bin/env bash
set -euo pipefail
SUBJ="$1"
NAME="$2"
openssl genrsa -out "${NAME}-key.pem" 3072
openssl req -new -x509 -key "${NAME}-key.pem" -out "${NAME}-cert.pem" -days 365 -subj "/${SUBJ}"
echo "Generated ${NAME}-key.pem and ${NAME}-cert.pem"