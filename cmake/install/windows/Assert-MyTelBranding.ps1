[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Executable,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '3.0.1',
    [string]$ExpectedIcon = (Join-Path $PSScriptRoot '../../../Linphone/data/icon.ico')
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$path = (Resolve-Path -LiteralPath $Executable).Path
$metadata = [Diagnostics.FileVersionInfo]::GetVersionInfo($path)
foreach ($name in @('ProductName', 'CompanyName')) {
    if ($metadata.$name -ne 'MyTel') { throw "$name is '$($metadata.$name)', expected MyTel." }
}
foreach ($name in @('FileVersion', 'ProductVersion')) {
    if ($metadata.$name -ne $Version) { throw "$name is '$($metadata.$name)', expected $Version." }
}
$embedded = [Drawing.Icon]::ExtractAssociatedIcon($path)
$expected = [Drawing.Icon]::new((Resolve-Path -LiteralPath $ExpectedIcon).Path, $embedded.Width, $embedded.Height)
$actualBitmap = $embedded.ToBitmap()
$expectedBitmap = $expected.ToBitmap()
try {
    $different = 0
    for ($y=0; $y -lt $actualBitmap.Height; $y++) {
        for ($x=0; $x -lt $actualBitmap.Width; $x++) {
            if ($actualBitmap.GetPixel($x,$y).ToArgb() -ne $expectedBitmap.GetPixel($x,$y).ToArgb()) { $different++ }
        }
    }
    if ($different -gt 0) { throw "PE icon differs from MyTel source artwork at $different pixels." }
    [pscustomobject]@{Executable=$path; Product=$metadata.ProductName; Publisher=$metadata.CompanyName;
        Version=$metadata.ProductVersion; IconSize="$($embedded.Width)x$($embedded.Height)"; IconMatches=$true}
} finally {
    $actualBitmap.Dispose(); $expectedBitmap.Dispose(); $embedded.Dispose(); $expected.Dispose()
}
