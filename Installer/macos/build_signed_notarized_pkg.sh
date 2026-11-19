#!/usr/bin/env bash
# BOOM commercial macOS release, adapted from Organix/Pedal Legends.
# Only Apple Accepted, stapled and Gatekeeper-verified PKGs are published.
set -euo pipefail
version="$1"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "::error::Invalid SemVer"; exit 1; }
bash Installer/macos/apple_secrets_preflight.sh
root="$PWD"
au="build/BOOM_artefacts/Release/AU/BOOM.component"
vst="build/BOOM_artefacts/Release/VST3/BOOM.vst3"
standalone="build/BOOM_artefacts/Release/Standalone/BOOM.app"
for bundle in "$au" "$vst" "$standalone"; do
  executable="$bundle/Contents/MacOS/BOOM"
  test -f "$executable" || { echo "::error::Missing BOOM executable in $bundle"; exit 1; }
  arches="$(/usr/bin/lipo -archs "$executable")"
  [[ " $arches " == *" arm64 "* && " $arches " == *" x86_64 "* ]] || {
    echo "::error::Not a universal arm64/x86_64 build: $bundle"; exit 1;
  }
done

keychain="$RUNNER_TEMP/boom-release.keychain-db"
pass="$(/usr/bin/uuidgen)"
appcert="$RUNNER_TEMP/boom-app.p12"
pkgcert="$RUNNER_TEMP/boom-installer.p12"
cleanup(){
  /bin/rm -f "$appcert" "$pkgcert"
  /usr/bin/security delete-keychain "$keychain" >/dev/null 2>&1 || true
}
trap cleanup EXIT
/usr/bin/security create-keychain -p "$pass" "$keychain"
/usr/bin/security set-keychain-settings -lut 21600 "$keychain"
/usr/bin/security unlock-keychain -p "$pass" "$keychain"
/usr/bin/security list-keychains -d user -s "$keychain"
/usr/bin/security default-keychain -s "$keychain"
printf '%s' "$APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64" | /usr/bin/base64 -D > "$appcert"
printf '%s' "$APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64" | /usr/bin/base64 -D > "$pkgcert"
/usr/bin/security import "$appcert" -k "$keychain" -P "$APPLE_P12_PASSWORD" -T /usr/bin/codesign
/usr/bin/security import "$pkgcert" -k "$keychain" -P "$APPLE_P12_PASSWORD" -T /usr/bin/pkgbuild
/usr/bin/security set-key-partition-list -S apple-tool:,apple: -s -k "$pass" "$keychain"
appIdentity="$(/usr/bin/security find-identity -v -p codesigning "$keychain" | /usr/bin/awk -F '"' '/Developer ID Application:/{print $2;exit}')"
pkgIdentity="$(/usr/bin/security find-identity -v "$keychain" | /usr/bin/awk -F '"' '/Developer ID Installer:/{print $2;exit}')"
[[ -n "$appIdentity" && -n "$pkgIdentity" ]] || { echo "::error::Missing Developer ID identity"; exit 1; }
[[ "$appIdentity" == *"($APPLE_NOTARY_TEAM_ID)"* && "$pkgIdentity" == *"($APPLE_NOTARY_TEAM_ID)"* ]] || {
  echo "::error::Apple certificates don't match the notarization Team ID"; exit 1;
}

stage="$RUNNER_TEMP/boom-stage"
out="$root/release-output"
logs="$root/release-validation"
mkdir -p "$stage/Library/Audio/Plug-Ins/Components" "$stage/Library/Audio/Plug-Ins/VST3" "$stage/Applications/Liberty Music Software" "$out" "$logs"
/usr/bin/ditto "$au" "$stage/Library/Audio/Plug-Ins/Components/BOOM.component"
/usr/bin/ditto "$vst" "$stage/Library/Audio/Plug-Ins/VST3/BOOM.vst3"
/usr/bin/ditto "$standalone" "$stage/Applications/Liberty Music Software/BOOM.app"
for bundle in "$stage/Library/Audio/Plug-Ins/Components/BOOM.component" "$stage/Library/Audio/Plug-Ins/VST3/BOOM.vst3" "$stage/Applications/Liberty Music Software/BOOM.app"; do
  /usr/bin/codesign --force --deep --options runtime --timestamp --sign "$appIdentity" "$bundle"
  /usr/bin/codesign --verify --deep --strict "$bundle"
  /usr/bin/codesign --display --verbose=4 "$bundle" 2>&1 | /usr/bin/grep -Fq 'Authority=Developer ID Application:'
done
pkg="$out/BOOM-v$version-macOS-Universal-SIGNED-NOTARIZED.pkg"
/usr/bin/pkgbuild --root "$stage" --identifier "com.libertymusicsoftware.boom.installer" --version "$version" --install-location / --ownership recommended --sign "$pkgIdentity" "$pkg"
/usr/sbin/pkgutil --check-signature "$pkg" | /usr/bin/tee "$logs/pkg-signature.txt"
/usr/sbin/pkgutil --payload-files "$pkg" > "$logs/pkg-payload.txt"
for file in 'Library/Audio/Plug-Ins/Components/BOOM.component/Contents/MacOS/BOOM' 'Library/Audio/Plug-Ins/VST3/BOOM.vst3/Contents/MacOS/BOOM' 'Applications/Liberty Music Software/BOOM.app/Contents/MacOS/BOOM'; do
  /usr/bin/grep -Fq "$file" "$logs/pkg-payload.txt" || { echo "::error::Package missing $file"; exit 1; }
done

notaryResult="$RUNNER_TEMP/boom-notary.json"
/usr/bin/xcrun notarytool submit "$pkg" --apple-id "$APPLE_NOTARY_APPLE_ID" --password "$APPLE_NOTARY_APP_PASSWORD" --team-id "$APPLE_NOTARY_TEAM_ID" --wait --output-format json > "$notaryResult"
status="$(/usr/bin/python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("status","Unknown"))' "$notaryResult")"
submission="$(/usr/bin/python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("id",""))' "$notaryResult")"
echo "Apple notarization status: $status"
if [[ -n "$submission" ]]; then
  echo "Apple submission ID: $submission"
  /usr/bin/xcrun notarytool log "$submission" --apple-id "$APPLE_NOTARY_APPLE_ID" --password "$APPLE_NOTARY_APP_PASSWORD" --team-id "$APPLE_NOTARY_TEAM_ID" "$logs/notary-log.json" >/dev/null 2>&1 || true
fi
[[ "$status" == "Accepted" ]] || { echo "::error::Apple notarization not accepted; commercial package withheld"; exit 1; }
/usr/bin/xcrun stapler staple "$pkg"
/usr/bin/xcrun stapler validate "$pkg" | /usr/bin/tee "$logs/stapler.txt"
/usr/sbin/spctl --assess --type install --verbose=2 "$pkg" 2>&1 | /usr/bin/tee "$logs/gatekeeper.txt"
sudo /usr/sbin/installer -pkg "$pkg" -target / | /usr/bin/tee "$logs/install.txt"
for bundle in '/Library/Audio/Plug-Ins/Components/BOOM.component' '/Library/Audio/Plug-Ins/VST3/BOOM.vst3' '/Applications/Liberty Music Software/BOOM.app'; do
  test -f "$bundle/Contents/MacOS/BOOM"
  /usr/bin/codesign --verify --deep --strict "$bundle"
done
test -s "$pkg"
/usr/bin/shasum -a 256 "$pkg" | /usr/bin/tee "$logs/SHA256.txt"
echo "::notice::BOOM universal AU/VST3/Standalone: Developer ID signed, Apple Accepted, ticket stapled, Gatekeeper and clean install verified"
