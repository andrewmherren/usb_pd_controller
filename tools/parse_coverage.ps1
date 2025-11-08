# Parse coverage.xml and display per-file statistics
# Usage: .\parse_coverage.ps1 [-CoverageXmlPath <path>]

[CmdletBinding()]
param(
    [string]$CoverageXmlPath
)

# Default to .coverage/coverage.xml relative to script location if not provided
if (-not $CoverageXmlPath) {
    $libRoot = Split-Path -Path $PSScriptRoot -Parent
    $CoverageXmlPath = Join-Path $libRoot ".coverage\coverage.xml"
}

if (-not (Test-Path $CoverageXmlPath)) {
    Write-Host "Coverage XML not found: $CoverageXmlPath" -ForegroundColor Red
    exit 1
}

[xml]$xml = Get-Content $CoverageXmlPath
$xml.coverage.packages.package.classes.class | Select-Object `
    @{N='File';E={$_.filename}}, `
    @{N='LineRate';E={[math]::Round([double]$_.'line-rate' * 100, 1)}}, `
    @{N='BranchRate';E={[math]::Round([double]$_.'branch-rate' * 100, 1)}} | `
    Format-Table -AutoSize
