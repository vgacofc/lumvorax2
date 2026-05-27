#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <manifest.json> <private_key.pem> [public_key.pem]"
  exit 2
fi

manifest="$1"
priv="$2"
pub="${3:-}"

openssl dgst -sha256 -sign "$priv" -out "${manifest}.sig" "$manifest"
echo "signature_written=${manifest}.sig"

if [[ -n "$pub" ]]; then
  openssl dgst -sha256 -verify "$pub" -signature "${manifest}.sig" "$manifest"
fi
