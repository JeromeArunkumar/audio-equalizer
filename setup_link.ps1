<#
.SYNOPSIS
    Wires the JUCE Audio Equalizer into the Equalizer APO processing pipeline.

.DESCRIPTION
    Appends the line "Include: juce_eq.txt" to Equalizer APO's master config file
    (config.txt) if that directive is not already present.

    This one-time setup step tells Equalizer APO to read the JUCE application's
    output file on every processing cycle, enabling real-time system-wide EQ.

    After this script runs:
      C:\Program Files\EqualizerAPO\config\config.txt
          ... (existing content) ...
          Include: juce_eq.txt          <-- appended by this script

    Then launch AudioEqualizer.exe (or the desktop shortcut) — any slider
    movement immediately rewrites juce_eq.txt and the change is audible.

.NOTES
    Requires: Equalizer APO installed (https://equalizerapo.sourceforge.net/)
    Run as:   Administrator  (config.txt lives under Program Files)
    Usage:    .\setup_link.ps1
#>

#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
$ApoConfigDir  = 'C:\Program Files\EqualizerAPO\config'
$ApoConfigFile = Join-Path $ApoConfigDir 'config.txt'
$IncludeLine   = 'Include: juce_eq.txt'

# ---------------------------------------------------------------------------
# Banner
# ---------------------------------------------------------------------------
Write-Host ''
Write-Host '╔══════════════════════════════════════════════════╗' -ForegroundColor Cyan
Write-Host '║     System Audio Equalizer  —  APO Link Setup    ║' -ForegroundColor Cyan
Write-Host '╚══════════════════════════════════════════════════╝' -ForegroundColor Cyan
Write-Host ''

# ---------------------------------------------------------------------------
# Check: Equalizer APO must already be installed
# ---------------------------------------------------------------------------
Write-Host 'Checking for Equalizer APO installation...' -NoNewline

if (-not (Test-Path $ApoConfigFile)) {
    Write-Host ' NOT FOUND' -ForegroundColor Red
    Write-Host @"

  ERROR: Equalizer APO config not found at:
    $ApoConfigFile

  Please install Equalizer APO first:
    https://equalizerapo.sourceforge.net/

  During setup, select your primary audio output device (headphones or speakers).
  Then re-run this script.
"@ -ForegroundColor Red
    exit 1
}

Write-Host ' OK' -ForegroundColor Green
Write-Host "  Path: $ApoConfigFile"

# ---------------------------------------------------------------------------
# Check: is the Include directive already present?
# ---------------------------------------------------------------------------
$existingContent = Get-Content -Path $ApoConfigFile -Raw -Encoding UTF8

if ($existingContent -imatch [regex]::Escape($IncludeLine)) {
    Write-Host ''
    Write-Host "Nothing to do — '$IncludeLine' is already in config.txt." `
               -ForegroundColor Yellow
    Write-Host ''
    Write-Host 'Launch AudioEqualizer.exe and move a slider to verify the link.' `
               -ForegroundColor Cyan
    exit 0
}

# ---------------------------------------------------------------------------
# Append the Include directive
# (Ensure the file ends with a newline before our line)
# ---------------------------------------------------------------------------
Write-Host ''
Write-Host "Appending '$IncludeLine' to config.txt..." -NoNewline

$needsNewlinePad = ($existingContent.Length -gt 0) -and
                   (-not ($existingContent.EndsWith("`n")))

$appendText = if ($needsNewlinePad) { "`r`n$IncludeLine`r`n" } else { "$IncludeLine`r`n" }

# Add-Content with -NoNewline lets us control the exact bytes appended.
Add-Content -Path $ApoConfigFile -Value $appendText -Encoding UTF8 -NoNewline

Write-Host ' Done.' -ForegroundColor Green

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Host @"

  Setup complete!

  Equalizer APO will now load:
    $ApoConfigDir\juce_eq.txt

  Next steps:
    1. Launch AudioEqualizer.exe (or the desktop shortcut).
    2. Move any slider — the change takes effect system-wide within ~10 ms.
    3. The EQ is active for ALL audio (Spotify, YouTube, system sounds, etc.)

  Tip: If you don't hear changes immediately, restart the Windows Audio service:
    net stop audiosrv && net start audiosrv
"@ -ForegroundColor Cyan
