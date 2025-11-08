# Local Coverage Generation Script
# Replicates SonarCloud CI coverage generation locally
# Outputs everything into a .coverage folder at the library root.
# Usage examples:
#   powershell -NoLogo -ExecutionPolicy Bypass -File .\tools\generate_coverage.ps1
#   powershell -NoLogo -ExecutionPolicy Bypass -File .\tools\generate_coverage.ps1 -Html

[CmdletBinding()]
param(
    [switch]$Html,
    [switch]$NoHtml
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Local Coverage Generation for SonarCloud" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ---------------------------------------------------------------------------
# Resolve paths relative to this script so it works from any CWD
# ---------------------------------------------------------------------------
$scriptDir = Split-Path -Path $PSCommandPath -Parent
$libRoot   = Split-Path -Path $scriptDir -Parent  # tools/ -> lib root

# ---------------------------------------------------------------------------
# Step 1: Prepare .coverage workspace (clean previous artifacts)
# ---------------------------------------------------------------------------
Write-Host "`n[1/6] Preparing .coverage workspace..." -ForegroundColor Yellow
$coverageRoot = Join-Path $libRoot ".coverage"
if (Test-Path $coverageRoot) {
    Remove-Item -LiteralPath $coverageRoot -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $coverageRoot | Out-Null
$gcovDestDir = Join-Path $coverageRoot "gcov"
New-Item -ItemType Directory -Path $gcovDestDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $coverageRoot "raw") | Out-Null

# Clean build to ensure fresh instrumentation
Remove-Item (Join-Path $libRoot ".pio\build\test_native") -Recurse -ErrorAction SilentlyContinue
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcda" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue }
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcov" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue }

# ---------------------------------------------------------------------------
# Step 2: Build and run tests
# ---------------------------------------------------------------------------
Write-Host "`n[2/6] Building and running tests..." -ForegroundColor Yellow

# Prefer pio from PATH; fall back to user-specific path if needed
$pioCmd = (Get-Command pio -ErrorAction SilentlyContinue)
if (-not $pioCmd) {
    $fallbackPio = "C:\\Users\\Drew\\.platformio\\penv\\Scripts\\pio.exe"
    if (Test-Path $fallbackPio) {
        $pioCmd = $fallbackPio
    } else {
        Write-Host "PlatformIO (pio) not found on PATH and fallback path missing." -ForegroundColor Red
        Write-Host "Please install PlatformIO and ensure 'pio' is available." -ForegroundColor Yellow
        exit 1
    }
}

$origLoc = Get-Location
Push-Location $libRoot
& $pioCmd test -e test_native
Pop-Location
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nTests failed! Fix test failures before checking coverage." -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Step 3: Run gcov to generate .gcov files
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
# Step 4: Generate coverage.xml using gcovr
# ---------------------------------------------------------------------------
Write-Host "`n[4/6] Generating coverage.xml report..." -ForegroundColor Yellow

$gcovrPath = Get-Command gcovr -ErrorAction SilentlyContinue
if (-not $gcovrPath) {
    Write-Host "  Installing gcovr..." -ForegroundColor Yellow
    pip install gcovr | Out-Null
}

$coverageXmlPath = Join-Path $coverageRoot "coverage.xml"
$summaryPath = Join-Path $coverageRoot "summary.txt"

gcovr --xml-pretty `
    --root $libRoot `
    --object-directory (Join-Path $libRoot ".pio/build/test_native") `
    --filter "src" --filter "include" `
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

# ---------------------------------------------------------------------------
# Step 5: Display summary
# ---------------------------------------------------------------------------
Write-Host "`n[5/6] Coverage report generated!" -ForegroundColor Green
Write-Host "`nCoverage artifacts:" -ForegroundColor Cyan
Write-Host "  - .coverage/coverage.xml (Sonar-compatible XML)" -ForegroundColor White
Write-Host "  - .coverage/summary.txt (human-readable summary)" -ForegroundColor White
Write-Host "  - .coverage/gcov/*.gcov (line-by-line raw coverage)" -ForegroundColor White

$coverageXml = [xml](Get-Content $coverageXmlPath)
$lineCoverage = $coverageXml.coverage.'line-rate'
$branchCoverage = $coverageXml.coverage.'branch-rate'
$linePercent = [math]::Round([double]$lineCoverage * 100, 1)
$branchPercent = [math]::Round([double]$branchCoverage * 100, 1)

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Coverage Summary:" -ForegroundColor Cyan
Write-Host "  Line Coverage:   $linePercent%" -ForegroundColor $(if ($linePercent -ge 80) { "Green" } else { "Yellow" })
Write-Host "  Branch Coverage: $branchPercent%" -ForegroundColor $(if ($branchPercent -ge 80) { "Green" } else { "Yellow" })
Write-Host "========================================" -ForegroundColor Cyan

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
        --filter "src" --filter "include" `
        --exclude-throw-branches `
        --exclude-directories ".*libdeps.*" `
        --exclude ".*FakeIt.*" `
        --exclude ".*Arduino.*" `
        --exclude ".*unity.*" `
        --exclude-unreachable-branches | Out-Null
        if (Test-Path $htmlPath) {
            Write-Host "HTML report generated: .coverage/coverage.html" -ForegroundColor Green
            Write-Host "Opening in browser..." -ForegroundColor Yellow
            Start-Process $htmlPath
        }
    }
}

Write-Host "`n[6/6] All artifacts written to .coverage" -ForegroundColor Green
Write-Host "`nDone! You can now:" -ForegroundColor Cyan
Write-Host "  1. View .coverage/coverage.xml in VS Code with Coverage Gutters extension" -ForegroundColor White
Write-Host "  2. Check .coverage/gcov/*.gcov for line-by-line coverage details" -ForegroundColor White
Write-Host "  3. Open .coverage/coverage.html for a detailed HTML report" -ForegroundColor White
Write-Host "  4. Compare with SonarCloud report" -ForegroundColor White