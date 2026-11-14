[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PayloadDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$MakeNsis,
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$ApplicationSourceSha,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '3.0.1'
)
$ErrorActionPreference = 'Stop'
$payload = (Resolve-Path -LiteralPath $PayloadDirectory).Path
$compiler = (Resolve-Path -LiteralPath $MakeNsis).Path
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../../..')).Path
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
if ($output.StartsWith($payload.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or $output -eq $payload) {
    throw 'Installer output must be outside the payload.'
}
$evidencePath = Join-Path $output 'installer-build-evidence.json'
if (Test-Path -LiteralPath $evidencePath) { Remove-Item -LiteralPath $evidencePath }
$desktopSourceSha = (& git -C $repo rev-parse --verify HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $desktopSourceSha -notmatch '^[0-9a-f]{40}$') { throw 'Cannot identify desktop source revision.' }
$sourceStatus = @(& git -C $repo status --porcelain)
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect desktop source worktree.' }
$desktopSourceClean = $sourceStatus.Count -eq 0
$exe = Join-Path $payload 'mytellphone.exe'
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Payload must contain mytellphone.exe.' }
$metadata = [Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
if ($metadata.ProductName -ne 'MyTel' -or $metadata.ProductVersion -ne $Version -or $metadata.CompanyName -ne 'MyTel') {
    throw "Payload is not the final MyTel $Version build. Product=$($metadata.ProductName), version=$($metadata.ProductVersion), publisher=$($metadata.CompanyName)"
}
& (Join-Path $PSScriptRoot 'Assert-MyTelBranding.ps1') -Executable $exe -Version $Version | Out-Null
$build = Join-Path $output 'installer-input'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$icon = Join-Path $repo 'Linphone/data/icon.ico'
$license = Join-Path $repo 'LICENSE.txt'
$header = Join-Path $PSScriptRoot 'mytel-header.bmp'
$welcome = Join-Path $PSScriptRoot 'mytel-welcome.bmp'
$files = @(Get-ChildItem -LiteralPath $payload -File -Recurse | Sort-Object FullName)
if ($files.Count -lt 10) { throw 'Payload is incomplete.' }
$relativeFiles = @($files | ForEach-Object { $_.FullName.Substring($payload.Length).TrimStart('\') })
if ($relativeFiles -contains 'Uninstall.exe') { throw 'Payload contains an unrelated uninstaller.' }
# Escape compile-time strings: payload names must never become NSIS instructions or variables.
function ConvertTo-NsisLiteral([string]$Value) {
    if ($Value.Contains('"') -or $Value.Contains("`r") -or $Value.Contains("`n")) { throw 'Unsupported path in payload.' }
    return $Value.Replace('$', '$$')
}
$installLines = [Collections.Generic.List[string]]::new()
$uninstallLines = [Collections.Generic.List[string]]::new()
$manifest = [Collections.Generic.List[object]]::new()
foreach ($file in $files) {
    $relative = $file.FullName.Substring($payload.Length).TrimStart('\')
    $directory = Split-Path -Path $relative -Parent
    $destination = if ($directory) { '$INSTDIR\' + (ConvertTo-NsisLiteral $directory) } else { '$INSTDIR' }
    $installLines.Add('SetOutPath "' + $destination + '"')
    $installLines.Add('File "' + (ConvertTo-NsisLiteral $file.FullName) + '"')
    $uninstallLines.Add('Delete "$INSTDIR\' + (ConvertTo-NsisLiteral $relative) + '"')
    $manifest.Add([ordered]@{path=$relative; bytes=$file.Length; sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()})
}
# Remove only known files and empty directories. Never remove a profile or recursively delete an installation tree.
Get-ChildItem -LiteralPath $payload -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending | ForEach-Object {
    $uninstallLines.Add('RMDir "$INSTDIR\' + (ConvertTo-NsisLiteral $_.FullName.Substring($payload.Length).TrimStart('\')) + '"')
}
$filesInclude = Join-Path $build 'files.nsh'
$uninstallInclude = Join-Path $build 'uninstall-files.nsh'
[IO.File]::WriteAllLines($filesInclude, $installLines, [Text.UTF8Encoding]::new($true))
[IO.File]::WriteAllLines($uninstallInclude, $uninstallLines, [Text.UTF8Encoding]::new($true))
$manifestPath = Join-Path $output 'installed-payload.json'
$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $manifestPath -Encoding utf8
$installer = Join-Path $output "MyTel-v$Version-windows-x64-setup.exe"
$sizeKB = [Math]::Ceiling(($files | Measure-Object Length -Sum).Sum / 1024)
$arguments = @('/V3', '/WX', "/DMYTEL_VERSION=$Version", "/DMYTEL_OUTPUT=$installer", "/DMYTEL_ICON=$icon",
    "/DMYTEL_HEADER=$header", "/DMYTEL_WELCOME=$welcome", "/DMYTEL_LICENSE=$license",
    "/DMYTEL_FILES=$filesInclude", "/DMYTEL_UNINSTALL_FILES=$uninstallInclude", "/DMYTEL_SIZE_KB=$sizeKB",
    (Join-Path $PSScriptRoot 'MyTel.nsi'))
& $compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "NSIS failed: $LASTEXITCODE" }
$sourceAfter = (& git -C $repo rev-parse --verify HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceAfter -ne $desktopSourceSha) { throw 'Desktop source revision changed during installer compilation.' }
$statusAfter = @(& git -C $repo status --porcelain)
if ($LASTEXITCODE -ne 0 -or ($statusAfter -join "`n") -ne ($sourceStatus -join "`n")) {
    throw 'Desktop source worktree changed during installer compilation.'
}
# Bind the exact compiled installer to this manifest, not a stale same-version artifact.
foreach ($entry in $manifest) {
    $file = Get-Item -LiteralPath (Join-Path $payload $entry.path)
    if ($file.Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Payload changed during installer compilation: $($entry.path)"
    }
}
$installerFile = Get-Item -LiteralPath $installer
$installerHash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
$evidence = [ordered]@{
    schemaVersion = 1
    version = $Version
    desktopSourceSha = $desktopSourceSha
    desktopSourceClean = $desktopSourceClean
    applicationSourceSha = $ApplicationSourceSha.ToLowerInvariant()
    installer = [ordered]@{name=$installerFile.Name; bytes=$installerFile.Length; sha256=$installerHash}
    payloadManifest = [ordered]@{name='installed-payload.json'; sha256=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()}
    nsisCompiler = [ordered]@{name=[IO.Path]::GetFileName($compiler); sha256=(Get-FileHash -LiteralPath $compiler -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$evidence | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $evidencePath -Encoding utf8
Get-FileHash -LiteralPath $installer -Algorithm SHA256
$installerFile
