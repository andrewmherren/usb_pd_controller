# Native Coverage Generation Script
# Generates coverage for NATIVE-TESTABLE code only (excludes platform-specific code)
# This matches what CI/SonarCloud reports
# Usage: .\tools\generate_native_coverage.ps1 [-Html] [-NoHtml]

[CmdletBinding()]
param(
    [switch]$Html,
    [switch]$NoHtml
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Native Coverage (Platform-Independent)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ---------------------------------------------------------------------------
# Resolve paths relative to this script
# ---------------------------------------------------------------------------
$scriptDir = Split-Path -Path $PSCommandPath -Parent
$libRoot   = Split-Path -Path $scriptDir -Parent

# ---------------------------------------------------------------------------
# Step 1: Prepare .coverage/native workspace
# ---------------------------------------------------------------------------
Write-Host "`n[1/6] Preparing .coverage/native workspace..." -ForegroundColor Yellow
$coverageRoot = Join-Path $libRoot ".coverage\native"
if (Test-Path $coverageRoot) {
    Remove-Item -LiteralPath $coverageRoot -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $coverageRoot | Out-Null
$gcovDestDir = Join-Path $coverageRoot "gcov"
New-Item -ItemType Directory -Path $gcovDestDir | Out-Null

# Clean build
Remove-Item (Join-Path $libRoot ".pio\build\test_native") -Recurse -ErrorAction SilentlyContinue
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcda" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue }
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcov" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue }

# ---------------------------------------------------------------------------
# Step 2: Build and run native tests
# ---------------------------------------------------------------------------
Write-Host "`n[2/6] Building and running native tests..." -ForegroundColor Yellow

$pioCmd = (Get-Command pio -ErrorAction SilentlyContinue)
if (-not $pioCmd) {
    $fallbackPio = "C:\\Users\\Drew\\.platformio\\penv\\Scripts\\pio.exe"
    if (Test-Path $fallbackPio) {
        $pioCmd = $fallbackPio
    } else {
        Write-Host "PlatformIO (pio) not found." -ForegroundColor Red
        exit 1
    }
}

Push-Location $libRoot
& $pioCmd test -e test_native
Pop-Location
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nTests failed!" -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Step 3: Run gcov
# ---------------------------------------------------------------------------
Write-Host "`n[3/6] Running gcov on instrumented files..." -ForegroundColor Yellow
$gcnoFiles = Get-ChildItem -Path (Join-Path $libRoot ".pio\build\test_native") -Filter "*.gcno" -Recurse -ErrorAction SilentlyContinue
$gcovCount = 0
foreach ($gcnoFile in $gcnoFiles) {
    if ($gcnoFile.FullName -notmatch "test[/\\]" -and $gcnoFile.FullName -notmatch "libdeps[/\\]") {
        $origLoc = Split-Path $gcnoFile.FullName -Parent
        Push-Location $origLoc
        gcov -b -l -p -c $gcnoFile.Name 2>$null | Out-Null
        Pop-Location
        $gcovCount++
    }
}
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcov" -ErrorAction SilentlyContinue | ForEach-Object {
    $destPath = Join-Path $gcovDestDir $_.Name
    Move-Item -LiteralPath $_.FullName -Destination $destPath -Force -ErrorAction SilentlyContinue
}
Write-Host "  Generated coverage for $gcovCount source files" -ForegroundColor Green

# ---------------------------------------------------------------------------
# Step 4: Generate coverage report (EXCLUDE platform-specific code)
# ---------------------------------------------------------------------------
Write-Host "`n[4/6] Generating native coverage report..." -ForegroundColor Yellow

$gcovrPath = Get-Command gcovr -ErrorAction SilentlyContinue
if (-not $gcovrPath) {
    Write-Host "  Installing gcovr..." -ForegroundColor Yellow
    pip install gcovr | Out-Null
}

$coverageXmlPath = Join-Path $coverageRoot "coverage.xml"
$sonarXmlPath    = Join-Path $coverageRoot "sonarqube.xml"
$summaryPath     = Join-Path $coverageRoot "summary.txt"

# EXCLUDE platform-specific code (#ifdef ARDUINO/ESP_PLATFORM)
gcovr --xml-pretty `
    --root $libRoot `
    --object-directory (Join-Path $libRoot ".pio/build/test_native") `
    --filter "^src/" --filter "^include/" `
    --exclude-throw-branches `
    --exclude-directories ".*libdeps.*" `
    --exclude ".*FakeIt.*" `
    --exclude ".*Arduino.*" `
    --exclude ".*unity.*" `
    --exclude "/usr/include/.*" `
    --exclude-lines-by-pattern ".*#ifdef (ARDUINO|ESP_PLATFORM).*" `
    --exclude-lines-by-pattern ".*#if defined\((ARDUINO|ESP_PLATFORM)\).*" `
    --exclude-unreachable-branches `
    -o $coverageXmlPath `
    --print-summary | Tee-Object -FilePath $summaryPath | Out-Null

if ($LASTEXITCODE -ne 0 -or -not (Test-Path $coverageXmlPath)) {
    Write-Host "`nFailed to generate coverage report!" -ForegroundColor Red
    exit 1
}

# Also generate SonarQube generic coverage report for CI ingestion
gcovr --sonarqube $sonarXmlPath `
    --root $libRoot `
    --object-directory (Join-Path $libRoot ".pio/build/test_native") `
    --filter "^src/" --filter "^include/" `
    --exclude-throw-branches `
    --exclude-directories ".*libdeps.*" `
    --exclude ".*FakeIt.*" `
    --exclude ".*Arduino.*" `
    --exclude ".*unity.*" `
    --exclude "/usr/include/.*" `
    --exclude-lines-by-pattern ".*#ifdef (ARDUINO|ESP_PLATFORM).*" `
    --exclude-lines-by-pattern ".*#if defined\((ARDUINO|ESP_PLATFORM)\).*" `
    --exclude-unreachable-branches | Out-Null

# ---------------------------------------------------------------------------
# Step 5: Display summary
# ---------------------------------------------------------------------------
Write-Host "`n[5/6] Native coverage report generated!" -ForegroundColor Green
Write-Host "`nCoverage artifacts:" -ForegroundColor Cyan
Write-Host "  - .coverage/native/coverage.xml" -ForegroundColor White
Write-Host "  - .coverage/native/summary.txt" -ForegroundColor White
Write-Host "  - .coverage/native/gcov/*.gcov" -ForegroundColor White

$coverageXml = [xml](Get-Content $coverageXmlPath)
$lineCoverage = $coverageXml.coverage.'line-rate'
$branchCoverage = $coverageXml.coverage.'branch-rate'
$linePercent = [math]::Round([double]$lineCoverage * 100, 1)
$branchPercent = [math]::Round([double]$branchCoverage * 100, 1)

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Native Coverage Summary:" -ForegroundColor Cyan
Write-Host "  Line Coverage:   $linePercent%" -ForegroundColor $(if ($linePercent -ge 80) { "Green" } else { "Yellow" })
Write-Host "  Branch Coverage: $branchPercent%" -ForegroundColor $(if ($branchPercent -ge 80) { "Green" } else { "Yellow" })
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  (Excludes platform-specific #ifdef ARDUINO/ESP_PLATFORM code)" -ForegroundColor DarkGray

# Display per-file breakdown
Write-Host "`nPer-File Coverage:" -ForegroundColor Cyan
$parseScript = Join-Path $scriptDir "parse_coverage.ps1"
if (Test-Path $parseScript) {
    & $parseScript -CoverageXmlPath $coverageXmlPath
} else {
    Write-Host "  (parse_coverage.ps1 not found)" -ForegroundColor Yellow
}

# ---------------------------------------------------------------------------
# Step 6: Optional HTML report
# ---------------------------------------------------------------------------
if (-not $NoHtml) {
    $doHtml = $Html
    if (-not $Html) {
        Write-Host "`nGenerate HTML coverage report? (y/n): " -ForegroundColor Yellow -NoNewline
        $response = Read-Host
        $doHtml = ($response -eq 'y' -or $response -eq 'Y')
    }
    if ($doHtml) {
        Write-Host "Generating HTML report..." -ForegroundColor Yellow
        $htmlPath = Join-Path $coverageRoot "coverage.html"
        gcovr --html-details $htmlPath `
            --root $libRoot `
            --object-directory (Join-Path $libRoot ".pio/build/test_native") `
            --filter "^src/" --filter "^include/" `
            --exclude-throw-branches `
            --exclude-directories ".*libdeps.*" `
            --exclude ".*FakeIt.*" `
            --exclude ".*Arduino.*" `
            --exclude ".*unity.*" `
            --exclude-lines-by-pattern ".*#ifdef (ARDUINO|ESP_PLATFORM).*" `
            --exclude-lines-by-pattern ".*#if defined\((ARDUINO|ESP_PLATFORM)\).*" `
            --exclude-unreachable-branches | Out-Null
        if (Test-Path $htmlPath) {
            Write-Host "HTML report: .coverage/native/coverage.html" -ForegroundColor Green
            Write-Host "Opening in browser..." -ForegroundColor Yellow
            Start-Process $htmlPath
        }
    }
}

Write-Host "`n[6/6] All artifacts written to .coverage/native" -ForegroundColor Green
Write-Host "`nThis shows coverage for platform-independent code testable in native environment." -ForegroundColor Cyan
Write-Host "For platform-specific code coverage, use generate_esp32_coverage.ps1" -ForegroundColor Cyan
