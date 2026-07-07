#Requires -RunAsAdministrator
# ensure_apo.ps1
# Stamps Equalizer APO FxProperties keys onto every audio render endpoint
# that is missing them. Safe to run repeatedly (idempotent).
# Scheduled via Task Scheduler: at logon + on device-connect event 20001.

$logPath = 'C:\ProgramData\AudioEqualizer\ensure_apo.log'
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
"[$(Get-Date -f 'yyyy-MM-dd HH:mm:ss')] ensure_apo started" | Out-File $logPath -Append

# ---------------------------------------------------------------------------
# Hardcoded APO FxProperties template (read from Speaker {2b897433...})
# Key format:  { Name ; Kind ; Value }
# Kinds: 1=String  7=MultiString
# ---------------------------------------------------------------------------
$apoPrefix = 'd04e05a6-594b-4fb6-a80d-01af5eed7d1d'

$template = @(
    [pscustomobject]@{ Name="{$apoPrefix},0";  Kind=[Microsoft.Win32.RegistryValueKind]::String;      Value='{00000000-0000-0000-0000-000000000000}' }
    [pscustomobject]@{ Name="{$apoPrefix},5";  Kind=[Microsoft.Win32.RegistryValueKind]::String;      Value='{EACD2258-FCAC-4FF4-B36D-419E924A6D79}' }
    [pscustomobject]@{ Name="{$apoPrefix},7";  Kind=[Microsoft.Win32.RegistryValueKind]::String;      Value='{EC1CC9CE-FAED-4822-828A-82A81A6F018F}' }
    [pscustomobject]@{ Name="{$apoPrefix},13"; Kind=[Microsoft.Win32.RegistryValueKind]::MultiString; Value=@('{905069CC-CF0D-4EAD-B7D7-FBC5A9E38BD5}') }
    [pscustomobject]@{ Name="{$apoPrefix},14"; Kind=[Microsoft.Win32.RegistryValueKind]::MultiString; Value=@('{90609662-991C-4873-BA68-8FF2D57B230B}') }
    [pscustomobject]@{ Name="{$apoPrefix},15"; Kind=[Microsoft.Win32.RegistryValueKind]::MultiString; Value=@('{8F3540DF-18B0-4943-B916-D4A79801587D}','{90705486-CFCC-4D2F-9671-B62846279A2C}') }
    [pscustomobject]@{ Name="{$apoPrefix},19"; Kind=[Microsoft.Win32.RegistryValueKind]::MultiString; Value=@('{90B02B1F-018F-47E5-B581-27DDE5EABBCC}') }
    [pscustomobject]@{ Name="{$apoPrefix},20"; Kind=[Microsoft.Win32.RegistryValueKind]::MultiString; Value=@('{90C0662B-E145-49D8-A0C1-670D079442BE}','{8F3540DF-18B0-4943-B916-D4A79801587D}') }
)

$refGuid    = '{2b897433-6942-4b51-993c-d2971a74a579}'
$renderBase = 'SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render'
$stamped    = 0
$skipped    = 0

$allGuids = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey($renderBase).GetSubKeyNames()

foreach ($guid in $allGuids) {
    if ($guid -ieq $refGuid) { $skipped++; continue }

    try {
        $fxRegPath = "$renderBase\$guid\FxProperties"
        $fxKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey($fxRegPath, $true)

        if ($fxKey -eq $null) {
            $parent = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("$renderBase\$guid", $true)
            if ($parent -eq $null) { "[$(Get-Date -f 'HH:mm:ss')] Skipped (no parent): $guid" | Out-File $logPath -Append; $skipped++; continue }
            $fxKey = $parent.CreateSubKey('FxProperties')
            $parent.Dispose()
        }

        $existing = ($fxKey.GetValueNames() | Where-Object { $_ -match $apoPrefix }).Count
        if ($existing -ge $template.Count) { $fxKey.Dispose(); $skipped++; continue }

        foreach ($entry in $template) {
            $fxKey.SetValue($entry.Name, $entry.Value, $entry.Kind)
        }
        $fxKey.Dispose()

        "[$(Get-Date -f 'HH:mm:ss')] Stamped: $guid" | Out-File $logPath -Append
        $stamped++
    }
    catch {
        "[$(Get-Date -f 'HH:mm:ss')] Skipped (error): $guid - $($_.Exception.Message)" | Out-File $logPath -Append
        $skipped++
    }
}

"[$(Get-Date -f 'HH:mm:ss')] Done. Stamped=$stamped Skipped=$skipped" | Out-File $logPath -Append
