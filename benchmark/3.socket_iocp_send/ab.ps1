param([string[]]$exes, [int]$rounds = 5, [int]$seconds = 3, [int]$threads = 8, [string]$filter = "", [string]$out = "ab.txt")
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$log = Join-Path $dir $out
"" | Set-Content $log
for ($r = 0; $r -lt $rounds; $r++) {
	foreach ($e in $exes) {
		$load = (Get-CimInstance Win32_Processor).LoadPercentage
		"== round $r $e load $load" | Add-Content $log
		if ($filter -ne "") { & (Join-Path $dir "$e.exe") $seconds $threads $filter 2>&1 | Add-Content $log }
		else { & (Join-Path $dir "$e.exe") $seconds $threads 2>&1 | Add-Content $log }
	}
}
# - medians per exe and case
$rows = @{}
$cur = ""
foreach ($line in Get-Content $log) {
	if ($line -match '^== round \d+ (\S+)') { $cur = $Matches[1]; continue }
	if ($line -match '^\[bench\] (.+?)\s+:\s+(\d+) msg/s cpu ([\d.]+) core ([\d.]+) us/msg') {
		$k = "$($Matches[1].Trim())|$cur"
		if (-not $rows.ContainsKey($k)) { $rows[$k] = @() }
		$rows[$k] += , @([double]$Matches[2], [double]$Matches[4])
	}
}
"`n== medians (msg/s, us/msg, min-max msg/s)" | Add-Content $log
foreach ($k in ($rows.Keys | Sort-Object)) {
	$v = $rows[$k]; $m = $v | ForEach-Object { $_[0] } | Sort-Object; $u = $v | ForEach-Object { $_[1] } | Sort-Object
	$mid = [int][math]::Floor($m.Count / 2)
	("{0,-70} {1,12:N0} {2,8:N3} ({3:N0}~{4:N0})" -f $k, $m[$mid], $u[$mid], $m[0], $m[-1]) | Add-Content $log
}
Get-Content $log | Select-String -Pattern '== medians' -Context 0, 100 | ForEach-Object { $_.Context.PostContext }
