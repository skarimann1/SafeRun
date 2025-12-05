#!/usr/bin/env bash
set -euo pipefail

SRC="$1"
SIGNER="$2"
OUTDIR="${3:-demo/bundle_$SIGNER}"
BIN="${SRC%.*}"
KEY="${SIGNER}-key.pem"
CERT="${SIGNER}-cert.pem"

mkdir -p "$OUTDIR"

g++ -O2 -o "$BIN" "examples/$SRC"

sha256sum "$BIN" | awk '{print $1}' > "$BIN.sha256"
openssl smime -sign -binary -in "$BIN.sha256" -signer "$CERT" -inkey "$KEY" -outform DER -out "$BIN.sig" -nodetach
rm "$BIN.sha256"

mv "$BIN" "$BIN.sig" "$OUTDIR/"
cp "$CERT" "$OUTDIR/cert.pem"

echo "Bundle ready: $OUTDIR"
ls -l "$OUTDIR"

echo ""
echo "Signer subject (for policy.yaml):"
openssl x509 -noout -subject -in "$OUTDIR/cert.pem" | sed 's/subject=//' | sed 's/ //g'