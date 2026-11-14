[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$MakeNsis,
    [Parameter(Mandatory)][string]$FixturePayload,
    [Parameter(Mandatory)][string]$Workspace,
    [string]$Template
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if (!$Template) { $Template = Join-Path $repo 'cmake/install/windows/MyTel.nsi' }
$source = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $Template).Path)
$testId = 'MyTel.InstallerTest.' + [Guid]::NewGuid().ToString('N')
$root = Join-Path ([IO.Path]::GetFullPath($Workspace)) $testId
New-Item -ItemType Directory -Force -Path $root | Out-Null
$payload = (Resolve-Path -LiteralPath $FixturePayload).Path
$compiler = (Resolve-Path -LiteralPath $MakeNsis).Path
$install = [Collections.Generic.List[string]]::new()
$remove = [Collections.Generic.List[string]]::new()
foreach ($file in Get-ChildItem -LiteralPath $payload -File) {
    if ($file.Name.Contains('$') -or $file.Name.Contains('"')) { throw 'Unsupported test fixture filename.' }
    $install.Add('File "' + $file.FullName + '"')
    $remove.Add('Delete "$INSTDIR\' + $file.Name + '"')
}
$files = Join-Path $root 'files.nsh'
$uninstallFiles = Join-Path $root 'uninstall-files.nsh'
[IO.File]::WriteAllLines($files, $install, [Text.UTF8Encoding]::new($true))
[IO.File]::WriteAllLines($uninstallFiles, $remove, [Text.UTF8Encoding]::new($true))
# Isolate side effects only: production ownership/validation/functions/sections are unchanged.
$isolated = $source.Replace('Software\MyTel', ('Software\' + $testId)).
    Replace('Uninstall\MyTel', ('Uninstall\' + $testId)).
    Replace('$SMPROGRAMS\MyTel', ('$SMPROGRAMS\' + $testId)).
    Replace('$DESKTOP\MyTel.lnk', ('$DESKTOP\' + $testId + '.lnk')).
    Replace('InstallDir "$LOCALAPPDATA\Programs\MyTel"', ('InstallDir "' + (Join-Path $root 'unused-default') + '"'))
$script = Join-Path $root 'fixture.nsi'
$exe = Join-Path $root 'MyTel-OwnershipFixture.exe'
[IO.File]::WriteAllText($script, $isolated, [Text.UTF8Encoding]::new($true))
$nsisArgs = @('/V2', '/WX', '/DMYTEL_VERSION=3.0.1', "/DMYTEL_OUTPUT=$exe",
    "/DMYTEL_ICON=$repo\Linphone\data\icon.ico", "/DMYTEL_HEADER=$repo\cmake\install\windows\mytel-header.bmp",
    "/DMYTEL_WELCOME=$repo\cmake\install\windows\mytel-welcome.bmp", "/DMYTEL_LICENSE=$repo\LICENSE.txt",
    "/DMYTEL_FILES=$files", "/DMYTEL_UNINSTALL_FILES=$uninstallFiles", '/DMYTEL_SIZE_KB=1000', $script)
& $compiler @nsisArgs
if ($LASTEXITCODE -ne 0) { throw 'Installer test fixture compilation failed.' }
function Invoke-Fixture([string]$Program, [string]$Arguments) {
    $info = [Diagnostics.ProcessStartInfo]::new($Program)
    $info.Arguments = $Arguments
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $process = [Diagnostics.Process]::Start($info)
    if (!$process.WaitForExit(30000)) {
        $process.Kill()
        throw 'Silent installer fixture unexpectedly waited for UI.'
    }
    $code = $process.ExitCode
    $process.Dispose()
    return $code
}
$checks = 0
$failures = 0
function Check([bool]$Condition, [string]$Name) {
    $script:checks++
    if (!$Condition) { $script:failures++; Write-Output "FAIL $Name" }
    else { Write-Output "PASS $Name" }
}
$target = Join-Path $root 'Fresh MyTel install'
$unowned = Join-Path $root 'unowned'
New-Item -ItemType Directory -Force -Path $unowned | Out-Null
$sentinel = Join-Path $unowned 'mytellphone.exe'
[IO.File]::WriteAllText($sentinel, 'UNRELATED FILE; MUST SURVIVE')
$code = Invoke-Fixture $exe "/S /D=$unowned"
Check ($code -eq 2 -and [IO.File]::ReadAllText($sentinel) -eq 'UNRELATED FILE; MUST SURVIVE') 'silent-D-existing-unowned-directory-blocked-without-overwrite'
$code = Invoke-Fixture $exe "/S /D=$target"
$marker = Join-Path $target '.mytel-install.ini'
$installed = Join-Path $target 'mytellphone.exe'
Check ($code -eq 0 -and (Test-Path -LiteralPath $installed) -and (Test-Path -LiteralPath $marker)) 'fresh-dedicated-directory-installed-with-ownership-marker'
if (Test-Path -LiteralPath $marker) {
    $markerBytes = [IO.File]::ReadAllBytes($marker)
    $unknown = Join-Path $target 'keep-unknown.txt'
    [IO.File]::WriteAllText($unknown, 'UNKNOWN FILE; MUST SURVIVE')
    [IO.File]::WriteAllText($installed, 'old version fixture')
    $code = Invoke-Fixture $exe "/S /D=$target"
    Check ($code -eq 0 -and (Get-FileHash -LiteralPath $installed).Hash -eq (Get-FileHash -LiteralPath (Join-Path $payload 'mytellphone.exe')).Hash -and (Test-Path -LiteralPath $unknown)) 'owned-upgrade-refreshes-known-files-preserves-unknown'
    [IO.File]::WriteAllText($marker, "[MyTel]`r`nIdentity=MyTelDesktop`r`nPath=$unowned`r`n")
    $code = Invoke-Fixture (Join-Path $target 'Uninstall.exe') "/S _?=$target"
    Check ($code -eq 2 -and (Test-Path -LiteralPath $installed) -and (Test-Path -LiteralPath $unknown)) 'uninstall-wrong-marker-path-blocked-before-delete'
    $code = Invoke-Fixture $exe "/S /D=$target"
    Check ($code -eq 2 -and (Test-Path -LiteralPath $installed)) 'install-existing-invalid-marker-blocked'
    [IO.File]::WriteAllBytes($marker, $markerBytes)
    $code = Invoke-Fixture (Join-Path $target 'Uninstall.exe') "/S _?=$target"
    Check ($code -eq 0 -and !(Test-Path -LiteralPath $installed) -and !(Test-Path -LiteralPath $marker) -and (Test-Path -LiteralPath $unknown)) 'owned-uninstall-deletes-known-files-preserves-unknown'
} else {
    Check $false 'ownership-marker-required-for-upgrade-and-uninstall-tests'
}
# Probe actual root validation in a no-write section. A failed guard can never write to these real roots.
if ($isolated.Contains('Function ${PREFIX}ValidateSafePath')) {
    $probe = $isolated.Replace('    Call ValidateInstallTarget' + "`n" + '    Call CheckMyTelClosed', '    Call ValidateSafePath' + "`n" + '    SetErrorLevel 0' + "`n" + '    Quit' + "`n" + '    Call CheckMyTelClosed')
    # Normalize CRLF before matching if the checkout uses Windows line endings.
    if ($probe -eq $isolated) {
        $probe = $isolated.Replace('    Call ValidateInstallTarget' + "`r`n" + '    Call CheckMyTelClosed', '    Call ValidateSafePath' + "`r`n" + '    SetErrorLevel 0' + "`r`n" + '    Quit' + "`r`n" + '    Call CheckMyTelClosed')
    }
    if ($probe -eq $isolated) { throw 'Could not isolate no-write root probe.' }
    # Bypass NSIS's separate built-in root check to exercise our own guard directly.
    $probe = $probe.Replace('Unicode true', ('Unicode true' + "`n" + 'AllowRootDirInstall true'))
    [IO.File]::WriteAllText($script, $probe, [Text.UTF8Encoding]::new($true))
    & $compiler @nsisArgs
    if ($LASTEXITCODE -ne 0) { throw 'No-write root probe compilation failed.' }
    foreach ($dangerous in @([IO.Path]::GetPathRoot($root), $env:WINDIR, ($env:WINDIR + '\'), $env:USERPROFILE, ($env:USERPROFILE + '\'), $env:LOCALAPPDATA)) {
        $code = Invoke-Fixture $exe "/S /D=$dangerous"
        Check ($code -eq 2) "no-write-actual-root-guard-blocked:$dangerous"
    }
}
else {
    # RED snapshots have no marker guard. Clean their unique test shortcuts/keys through their own uninstaller.
    foreach ($directory in @($unowned, $target)) {
        $uninstaller = Join-Path $directory 'Uninstall.exe'
        if (Test-Path -LiteralPath $uninstaller) { $null = Invoke-Fixture $uninstaller "/S _?=$directory" }
    }
}
Write-Output "$($checks-$failures)/$checks actual NSIS ownership guard checks passed. Evidence fixtures: $root"
if ($failures -gt 0) { exit 1 }
