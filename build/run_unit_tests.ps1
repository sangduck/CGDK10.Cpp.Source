#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                        run unit tests (windows)                           *
#*                                                                           *
#*****************************************************************************
#
# usage) powershell -ExecutionPolicy Bypass -File build\run_unit_tests.ps1 [-Configuration Debug|Release] [-ResultDir <dir>] [-Exclude <pattern>,...]
#
#  - runs the unit tests built by Visual Studio 18 (build\src\unit_test\<configuration>\unit_test.*.vs18_*_<configuration>.exe)
#  - Exclude : regex patterns of the names not to run (default : 'bench' (benchmarks), 'server_query' (needs a database))
#  - results are written as JUnit XML (gtest xml) : <result directory>\<name>.xml, output : <result directory>\<name>.log
#  - a crashed / timed out executable is written as a failed test case : <result directory>\<name>.process.xml
#  - only the results of the previous run (*.xml, *.log) are removed (a build/source directory is refused)
#  - exit code : 0 all passed, 1 failed, 2 no unit test executable or invalid result directory
#  - UNIT_TEST_TIMEOUT : timeout of each executable (seconds, default 600)
#
#-----------------------------------------------------------------------------
param(
	[string]$Configuration = "Debug",
	[string]$ResultDir = "",
	[string[]]$Exclude = @("bench", "server_query")
)

$root_dir = Split-Path -Parent $PSScriptRoot
$bin_dir = Join-Path $root_dir "build\src\unit_test\$Configuration"
if ($ResultDir -eq "") { $ResultDir = Join-Path $root_dir "test-results-windows" }
$timeout = if ($env:UNIT_TEST_TIMEOUT) { [int]$env:UNIT_TEST_TIMEOUT } else { 600 }

# 1) result directory
foreach ($mark in @("CMakeCache.txt", "CMakeLists.txt", ".svn"))
{
	if (Test-Path (Join-Path $ResultDir $mark))
	{
		Write-Host "invalid result directory (has $mark): $ResultDir"
		exit 2
	}
}
New-Item -ItemType Directory $ResultDir -Force | Out-Null
Remove-Item (Join-Path $ResultDir "*.xml"), (Join-Path $ResultDir "*.log") -Force -ErrorAction SilentlyContinue
$ResultDir = (Resolve-Path $ResultDir).Path

# 2) unit test executables
$executables = @(Get-ChildItem (Join-Path $bin_dir "unit_test.*.vs18_*_$Configuration.exe") -ErrorAction SilentlyContinue |
	Where-Object { $name = $_.Name; -not ($Exclude | Where-Object { $name -match $_ }) } | Sort-Object Name)

if ($executables.Count -eq 0)
{
	Write-Host "no unit test executable : $bin_dir"
	exit 2
}

# 3) run
$failed = 0
foreach ($exe in $executables)
{
	$name = $exe.Name -replace "^unit_test\.", "" -replace "\.vs18_.*$", ""
	$file_xml = Join-Path $ResultDir "$name.xml"
	$file_log = Join-Path $ResultDir "$name.log"
	Write-Host ("=== [{0}] {1}" -f $name, (Get-Date -Format "HH:mm:ss"))

	$time_start = Get-Date
	$process = Start-Process -FilePath $exe.FullName -ArgumentList "--gtest_output=xml:`"$file_xml`"" -WorkingDirectory $exe.DirectoryName `
		-RedirectStandardOutput $file_log -RedirectStandardError "$file_log.err" -NoNewWindow -PassThru
	$null = $process.Handle			# - keeps the handle : ExitCode is empty without it (WaitForExit with a timeout)
	$is_exited = $process.WaitForExit($timeout * 1000)
	if (-not $is_exited) { $process.Kill(); $process.WaitForExit() }
	$exit_code = if ($is_exited) { $process.ExitCode } else { -1 }
	$elapsed = [int]((Get-Date) - $time_start).TotalSeconds

	# - stderr is appended to the log
	if (Test-Path "$file_log.err") { Get-Content "$file_log.err" | Add-Content $file_log; Remove-Item "$file_log.err" -Force }

	$summary = Select-String -Path $file_log -Pattern "^\[  (PASSED|FAILED|SKIPPED)  \]" -ErrorAction SilentlyContinue | ForEach-Object { "    " + $_.Line }
	if ($summary) { $summary | ForEach-Object { Write-Host $_ } }
	Write-Host "    exit=$exit_code (${elapsed}s)"

	if ($exit_code -ne 0)
	{
		$failed = 1

		# - crashed / timed out (no xml) : a failed test case
		if (-not (Test-Path $file_xml))
		{
			$reason = if ($is_exited) { "exit code $exit_code" } else { "timeout (${timeout}s)" }
			$text = "<?xml version=`"1.0`" encoding=`"UTF-8`"?>`n<testsuites tests=`"1`" failures=`"1`"><testsuite name=`"$name`" tests=`"1`" failures=`"1`"><testcase name=`"process`" classname=`"$name`"><failure message=`"$reason`"/></testcase></testsuite></testsuites>`n"
			Set-Content -Path (Join-Path $ResultDir "$name.process.xml") -Value $text -Encoding UTF8
		}
	}
}

Write-Host ("=== {0} unit test executables : {1}" -f $executables.Count, $(if ($failed) { "failed" } else { "0 failed" }))
exit $failed
