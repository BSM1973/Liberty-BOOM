#!/usr/bin/env bash
# Same Apple secrets as the Organix/Pedal Legends one-click release.
set -euo pipefail
missing=0
for key in APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64 APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64 APPLE_P12_PASSWORD APPLE_NOTARY_APPLE_ID APPLE_NOTARY_APP_PASSWORD APPLE_NOTARY_TEAM_ID; do
  value="$(printenv "$key" || true)"
  if [[ -z "$value" ]]; then
    echo "::error::Missing GitHub Actions secret: $key"
    missing=1
  fi
done
if (( missing )); then
  echo "::error::BOOM signing blocked. Add the six Apple secrets to the BOOM repository Actions settings."
  exit 1
fi
for key in APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64 APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64; do
  if ! printenv "$key" | /usr/bin/base64 -D >/dev/null 2>&1; then
    echo "::error::Certificate secret $key is not base64-decodable"
    exit 1
  fi
done
echo "::notice::Six Apple secrets present and certificates decoded successfully. No secret data logged."
