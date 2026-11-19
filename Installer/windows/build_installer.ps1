# BOOM Windows 64-bit VST3 installer, using Pedal Legends Inno Setup method.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Version)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$inputDir = Join-Path $root "build/BOOM_artefacts/Release/VST3"
if (-not (Test-Path -LiteralPath $inputDir -PathType Container)) {
  throw "Missing built BOOM VST3 folder"
}
$bundles = @(Get-ChildItem -LiteralPath $inputDir -Directory -Filter "*.vst3")
if ($bundles.Count -ne 1 -or $bundles[0].Name -ne "BOOM.vst3") {
  throw "Expected exactly one BOOM.vst3 bundle; found $($bundles.Count)"
}
$bin = @(Get-ChildItem -LiteralPath $bundles[0].FullName -Recurse -File -Filter "*.vst3")
if ($bin.Count -lt 1) { throw "No native VST3 executable inside the bundle" }
$output = Join-Path $root "release-output"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$env:BOOM_RELEASE_VERSION = $Version
$env:BOOM_VST3_BUNDLE = $bundles[0].FullName
$env:BOOM_INSTALLER_OUTPUT = $output
$iscc = $null
$locations = @(
 (Join-Path $env:ProgramFiles "Inno Setup 6/ISCC.exe"),
 (Join-Path ([Environment]::GetFolderPath("ProgramFilesX86")) "Inno Setup 6/ISCC.exe")
)
foreach ($candidate in $locations) {
 if (Test-Path -LiteralPath $candidate) { $iscc = $candidate; break }
}
if (-not $iscc) {
 $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
 if ($command) { $iscc = $command.Source }
}
if (-not $iscc) {
 choco install innosetup -y --no-progress --limit-output
 if ($LASTEXITCODE -ne 0) { throw "Could not install Inno Setup" }
 foreach ($candidate in $locations) {
  if (Test-Path -LiteralPath $candidate) { $iscc = $candidate; break }
 }
}
if (-not $iscc) { throw "Inno Setup ISCC.exe not found" }
& $iscc (Join-Path $PSScriptRoot "BOOM.iss")
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed with code $LASTEXITCODE" }
$installer = Join-Path $output "BOOM-v$Version-Windows-x64-VST3-Installer-unsigned.exe"
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) { throw "No installer created" }
if ((Get-Item -LiteralPath $installer).Length -lt 100000) { throw "Invalid installer size" }
Get-FileHash -Algorithm SHA256 -LiteralPath $installer | Format-List Path,Hash
Write-Host "Windows VST3 installer generated: $installer"
