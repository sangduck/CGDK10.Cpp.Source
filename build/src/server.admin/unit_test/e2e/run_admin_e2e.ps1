#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                       Ver 10.0 / Release 2019.12.11                       *
#*                                                                           *
#*            e2e test : service -> host_agent -> admin_server               *
#*                                                                           *
#*****************************************************************************
#
# usage)
#   powershell -ExecutionPolicy Bypass -File run_admin_e2e.ps1 [-Configuration Debug|Release] [-TestExe <path>]
#
# scenarios)
#   1) basic          : desired 'running' -> service(unit test) initialized -> admin sends START -> RUNNING
#   2) certify_key    : wrong key is rejected, right key is accepted
#   3) host_restart   : host_agent restarts -> admin unregisters and registers again
#   4) crash_loop     : service exits repeatedly -> auto restart is suspended
#   5) control_api    : http control api (key, list, desired, metrics) and admin setting reload
#   6) host_reload    : host_agent reloads host_agent_server_list.json / host_agent_setting.json (web ui policy)
#   7) security       : websocket Origin check, control api auth lockout (429), host status (/api/hosts, /metrics)
#   8) web_login      : host_agent web ui login ("login_key"), session cookie
#   9) admin_restart  : admin_server restarts -> host_agent reconnects and registers again, host status restored
#
# notes)
#   - ports 21887/21888(admin) / 22044/22052/22053(host_agent) must be free.
#     (environment variable CGDK_E2E_PORT_OFFSET is added to all ports. ex) 100 -> 21987/21988/22144/22152/22153)
#   - admin_server/host_agent run in a temporary directory with their own settings.
#   - the service is 'service_admin.e2e_through_host_agent' of unit_test.server_system.service
#     (rank 0.0.10.1 = iid 21101 of host_agent_server_list.json).
#-----------------------------------------------------------------------------
param(
	[string]$Configuration = "Debug",
	[string]$TestExe = ""
)

$ErrorActionPreference = "Stop"

#-----------------------------------------------------------------------------
# ports (CGDK_E2E_PORT_OFFSET is added. several runs can be done at the same time on a node)
#-----------------------------------------------------------------------------
$port_offset = if ($env:CGDK_E2E_PORT_OFFSET) { [int]$env:CGDK_E2E_PORT_OFFSET } else { 0 }
$port_admin = 21887 + $port_offset
$port_control = 21888 + $port_offset
$port_host = 22044 + $port_offset
$port_web = 22052 + $port_offset
$port_websocket = 22053 + $port_offset

#-----------------------------------------------------------------------------
# paths
#-----------------------------------------------------------------------------
$root_build = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path		# build/
$dir_execute = Join-Path $root_build "execute"

$suffix = if ($Configuration -eq "Debug") { ".Debug" } else { "" }
$exe_host_agent = Join-Path $dir_execute "CGDK10.server.admin.host_agent$suffix.exe"
$exe_admin = Join-Path $dir_execute "CGDK10.server_admin.admin_server.console\CGDK10.server_admin.admin_server.console$suffix.exe"

if ($TestExe -eq "")
{
	$TestExe = Get-ChildItem (Join-Path $root_build "src\unit_test\$Configuration") -Filter "unit_test.server_system.service*_$Configuration.exe" |
		Sort-Object LastWriteTime -Descending | Select-Object -First 1 | ForEach-Object { $_.FullName }
}

foreach ($file in @($exe_host_agent, $exe_admin, $TestExe))
{
	if ([string]::IsNullOrEmpty($file) -or -not (Test-Path $file)) { throw "not found: '$file'" }
}

$dir_work = Join-Path ([System.IO.Path]::GetTempPath()) "cgdk_admin_e2e_$port_offset"
if (Test-Path $dir_work) { Remove-Item $dir_work -Recurse -Force }
New-Item -ItemType Directory $dir_work | Out-Null

#-----------------------------------------------------------------------------
# helpers
#-----------------------------------------------------------------------------
$script:results = @()

function Write-Utf8([string]$_path, [string]$_text)
{
	$dir = Split-Path $_path -Parent
	if (-not (Test-Path $dir)) { New-Item -ItemType Directory $dir | Out-Null }
	$_text = $_text.Replace("{PORT_ADMIN}", "$port_admin").Replace("{PORT_CONTROL}", "$port_control").Replace("{PORT_HOST}", "$port_host").Replace("{PORT_WEB}", "$port_web").Replace("{PORT_WEBSOCKET}", "$port_websocket")
	[System.IO.File]::WriteAllText($_path, $_text, (New-Object System.Text.UTF8Encoding($false)))
}

function New-AdminDir([string]$_name, [string]$_manager, [string]$_acceptor_extra, [string]$_desired, [string]$_control_extra = "", [string]$_root_extra = "")
{
	$dir = Join-Path $dir_work "$_name\admin"
	$setting = @"
{
  "service": {
    "setting": { "name": "admin_server" },
    "logger": { "logger_file": { "path": "log", "filename": "admin_server.log" } }
  },
  "service_instance_manager": { $_manager },
  $_root_extra
  "acceptor_control": { "local_endpoint": { "address": "127.0.0.1", "port": {PORT_CONTROL} }, $_control_extra "prepare_on_start": 2, "must_prepare": 2 },
  "acceptor_host_agent": {
    "local_endpoint": { "port": {PORT_ADMIN} },
    $_acceptor_extra
    "prepare_on_start": 2,
    "must_prepare": 2
  }
}
"@
	Write-Utf8 (Join-Path $dir "config\admin_server.setting.json") $setting
	if ($_desired -ne "") { Write-Utf8 (Join-Path $dir "config\admin_server.desired_state.json") $_desired }
	return $dir
}

function New-HostAgentDir([string]$_name, [string]$_extra, [string]$_web_extra = "", [string]$_websocket_extra = "")
{
	$dir = Join-Path $dir_work "$_name\host_agent"
	New-Item -ItemType Directory $dir -Force | Out-Null
	$setting = @"
{
  "service": {},
  "system_executor": { "thread_count": 2 },
  "socket_to_admin_server": { "remote_endpoint": { "address": "127.0.0.1", "port": {PORT_ADMIN} } },
  "acceptor_server": { "local_endpoint": { "Port": {PORT_HOST} }, $_extra "prepare_on_start": 2, "must_prepare": 2 },
  "acceptor_web": { "local_endpoint": { "Port": {PORT_WEB} }, $_web_extra "prepare_on_start": 2, "must_prepare": 2 },
  "acceptor_websocket": { "local_endpoint": { "Port": {PORT_WEBSOCKET} }, $_websocket_extra "prepare_on_start": 2, "must_prepare": 2 }
}
"@
	$list = @"
{
  "e2e service": {
    "iid": 21101,
    "rank": { "group": 0, "field": 0, "role": 10, "index": 1 },
    "execute_file": "not_exist_e2e_service.exe",
    "path": "./",
    "working_path": "./"
  }
}
"@
	Write-Utf8 (Join-Path $dir "host_agent_setting.json") $setting
	Write-Utf8 (Join-Path $dir "host_agent_server_list.json") $list
	return $dir
}

function Start-Server([string]$_exe, [string]$_dir, [hashtable]$_env = @{})
{
	$saved = @{}
	foreach ($key in $_env.Keys) { $saved[$key] = [Environment]::GetEnvironmentVariable($key); [Environment]::SetEnvironmentVariable($key, $_env[$key]) }
	try
	{
		return Start-Process -FilePath $_exe -WorkingDirectory $_dir -WindowStyle Hidden -PassThru
	}
	finally
	{
		foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key]) }
	}
}

function Stop-Server($_process)
{
	if ($null -ne $_process -and -not $_process.HasExited)
	{
		Stop-Process -Id $_process.Id -Force -ErrorAction SilentlyContinue
		$_process.WaitForExit(5000) | Out-Null
	}
}

function Get-AdminLog([string]$_dir)
{
	$text = ""
	foreach ($file in Get-ChildItem (Join-Path $_dir "log") -Filter "*.log" -ErrorAction SilentlyContinue)
	{
		# - shared read (the file is opened by admin_server)
		$stream = New-Object System.IO.FileStream($file.FullName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
		$reader = New-Object System.IO.StreamReader($stream)
		$text += $reader.ReadToEnd()
		$reader.Close()
	}
	return $text
}

function Wait-AdminLog([string]$_dir, [string]$_pattern, [int]$_timeout_seconds = 15, [int]$_count = 1)
{
	$deadline = (Get-Date).AddSeconds($_timeout_seconds)
	while ((Get-Date) -lt $deadline)
	{
		$matches_found = [regex]::Matches((Get-AdminLog $_dir), $_pattern).Count
		if ($matches_found -ge $_count) { return $true }
		Start-Sleep -Milliseconds 200
	}
	return $false
}

function Invoke-Service([hashtable]$_env = @{})
{
	$saved = @{}
	$_env["CGDK_TEST_HOST_AGENT"] = "1"
	$_env["CGDK_TEST_HOST_AGENT_PORT"] = "$port_host"
	foreach ($key in $_env.Keys) { $saved[$key] = [Environment]::GetEnvironmentVariable($key); [Environment]::SetEnvironmentVariable($key, $_env[$key]) }
	try
	{
		$output = & $TestExe "--gtest_filter=service_admin.e2e_through_host_agent" 2>&1 | Out-String
		return ($output -match "\[  PASSED  \] 1 test")
	}
	finally
	{
		foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key]) }
	}
}

function Invoke-Http([string]$_method, [string]$_url, [string]$_key = "", [string]$_host = "")
{
	# - returns @{ code; body } (code 0: connection failed)
	$args_curl = @("-s", "-m", "5", "-X", $_method, "-o", "-", "-w", "`n%{http_code}")
	if ($_key -ne "") { $args_curl += @("-H", "X-CGDK-Key: $_key") }
	if ($_host -ne "") { $args_curl += @("-H", "Host: $_host") }
	$output = (& curl.exe @args_curl $_url 2>$null | Out-String).TrimEnd()
	$pos = $output.LastIndexOf("`n")
	$code = if ($pos -ge 0) { $output.Substring($pos + 1) } else { $output }
	$body = if ($pos -ge 0) { $output.Substring(0, $pos) } else { "" }
	return @{ code = [int]("0" + $code.Trim()); body = $body }
}

function Invoke-WebSocket([string]$_origin, [string]$_cookie = "")
{
	# - websocket handshake with Origin. returns the http status (101: accepted, 0: closed)
	$args_cookie = if ($_cookie -ne "") { @("-H", "Cookie: $_cookie") } else { @() }
	$output = (& curl.exe -s -m 3 -o NUL -w "%{http_code}" -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Sec-WebSocket-Version: 13" -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" -H "Origin: $_origin" @args_cookie "http://127.0.0.1:$port_websocket/log" 2>$null | Out-String).Trim()
	return [int]("0" + $output)
}

function Test-WebSocketKeepalive([string]$_origin)
{
	# - websocket (ping_interval 1s) : the server sends PING, no PONG -> closed in time
	$client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $port_websocket)
	try
	{
		$stream = $client.GetStream()
		$request = "GET /log HTTP/1.1`r`nHost: 127.0.0.1:$port_websocket`r`nConnection: Upgrade`r`nUpgrade: websocket`r`nSec-WebSocket-Version: 13`r`nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==`r`nOrigin: $_origin`r`n`r`n"
		$bytes = [System.Text.Encoding]::ASCII.GetBytes($request)
		$stream.Write($bytes, 0, $bytes.Length)
		$stream.ReadTimeout = 10000

		$time_start = Get-Date
		$received = New-Object System.Collections.Generic.List[byte]
		$buffer = New-Object byte[] 4096
		while (((Get-Date) - $time_start).TotalSeconds -lt 15)
		{
			$count = 0
			try { $count = $stream.Read($buffer, 0, $buffer.Length) } catch { break }
			if ($count -le 0) { break }
			for ($i = 0; $i -lt $count; ++$i) { $received.Add($buffer[$i]) }
		}
		$elapsed = ((Get-Date) - $time_start).TotalSeconds

		# - 101 / PING (0x89 0x00) after the response header / closed (not the read timeout)
		$data = $received.ToArray()
		$text = [System.Text.Encoding]::ASCII.GetString($data)
		$pos_body = $text.IndexOf("`r`n`r`n")
		$has_ping = $false
		if ($pos_body -ge 0)
		{
			for ($i = $pos_body + 4; $i -lt $data.Length - 1; ++$i) { if ($data[$i] -eq 0x89 -and $data[$i + 1] -eq 0) { $has_ping = $true; break } }
		}
		return ($text.StartsWith("HTTP/1.1 101") -and $has_ping -and $elapsed -lt 12)
	}
	finally { $client.Close() }
}

function Test-SlowRequestClosed([int]$_port)
{
	# - sends a part of a request and waits : closed by the server (request_timeout) in time -> true
	$client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $_port)
	try
	{
		$stream = $client.GetStream()
		$bytes = [System.Text.Encoding]::ASCII.GetBytes("GET /metrics HTTP/1.1`r`nX-Slow: ")
		$stream.Write($bytes, 0, $bytes.Length)
		$stream.ReadTimeout = 10000
		$time_start = Get-Date
		try { [void]$stream.Read((New-Object byte[] 16), 0, 16) } catch {}
		return (((Get-Date) - $time_start).TotalSeconds -lt 9)
	}
	finally { $client.Close() }
}

function Wait-Http([string]$_method, [string]$_url, [string]$_key, [int]$_code, [string]$_pattern = "", [int]$_timeout_seconds = 15)
{
	$deadline = (Get-Date).AddSeconds($_timeout_seconds)
	while ((Get-Date) -lt $deadline)
	{
		$response = Invoke-Http $_method $_url $_key
		if ($response.code -eq $_code -and ($_pattern -eq "" -or $response.body -match $_pattern)) { return $true }
		Start-Sleep -Milliseconds 500
	}
	return $false
}

function Add-Result([string]$_scenario, [string]$_check, [bool]$_ok)
{
	$script:results += [pscustomobject]@{ scenario = $_scenario; check = $_check; result = $(if ($_ok) { "PASS" } else { "FAIL" }) }
	Write-Host ("  [{0}] {1} - {2}" -f $(if ($_ok) { "PASS" } else { "FAIL" }), $_scenario, $_check)
}

#-----------------------------------------------------------------------------
# scenarios
#-----------------------------------------------------------------------------
$control = "http://127.0.0.1:$port_control"
$desired_running = '{ "21101": "running" }'

# 1) basic
Write-Host "1) basic"
$dir_admin = New-AdminDir "basic" '"auto_start": false' '' $desired_running
$dir_host = New-HostAgentDir "basic" ''
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Add-Result "basic" "host registered" (Wait-AdminLog $dir_admin "HOST\['.*'\] registered")
	Add-Result "basic" "service reached RUNNING" (Invoke-Service)
	Add-Result "basic" "START result with request id" (Wait-AdminLog $dir_admin "request #\d+ START result: SUCCESS")
	Add-Result "basic" "INIT of missing executable fails" (Wait-AdminLog $dir_admin "request #\d+ INIT result: FAIL")
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 2) certify key
Write-Host "2) certify_key"
$dir_admin = New-AdminDir "certify_key" '"auto_start": false' '"certify_key": "e2e-key",' ''
$dir_host = New-HostAgentDir "certify_key" ''
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host @{ "CGDK_ADMIN_CERTIFY_KEY" = "wrong-key" }
try
{
	Add-Result "certify_key" "wrong key rejected" (Wait-AdminLog $dir_admin "certify key mismatch")
	# - set the right key to the setting file (host_agent reloads it and reconnects. no restart)
	Start-Sleep 1
	$file_setting = Join-Path $dir_host "host_agent_setting.json"
	Write-Utf8 $file_setting ((Get-Content $file_setting -Raw) -replace '"socket_to_admin_server": \{', '"socket_to_admin_server": { "certify_key": "e2e-key",')
	Add-Result "certify_key" "right key accepted (setting reload)" (Wait-AdminLog $dir_admin "HOST\['.*'\] registered" 30)
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 3) host_agent restart
Write-Host "3) host_restart"
$dir_admin = New-AdminDir "host_restart" '"auto_start": false' '' ''
$dir_host = New-HostAgentDir "host_restart" ''
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Add-Result "host_restart" "first registration" (Wait-AdminLog $dir_admin "> HOST\['.*'\] registered")
	Stop-Server $p_host
	Add-Result "host_restart" "unregistered on disconnect" (Wait-AdminLog $dir_admin "< HOST\['.*'\] unregistered")
	$p_host = Start-Server $exe_host_agent $dir_host
	Add-Result "host_restart" "registered again" (Wait-AdminLog $dir_admin "> HOST\['.*'\] registered" 15 2)
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 4) crash loop (service exits repeatedly)
Write-Host "4) crash_loop"
$dir_admin = New-AdminDir "crash_loop" '"auto_start": false, "restart_interval": 0, "restart_max_count": 2, "restart_window": 60' '' $desired_running
$dir_host = New-HostAgentDir "crash_loop" ''
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Wait-AdminLog $dir_admin "HOST\['.*'\] registered" | Out-Null
	for ($i = 0; $i -lt 3; ++$i) { Invoke-Service | Out-Null; Start-Sleep -Milliseconds 500 }
	Add-Result "crash_loop" "auto restart suspended" (Wait-AdminLog $dir_admin "auto restart is suspended")
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 5) control api / admin setting reload
Write-Host "5) control_api"
$dir_admin = New-AdminDir "control_api" '"auto_start": false' '' '' '"certify_key": "ctl-key",'
$dir_host = New-HostAgentDir "control_api" ''
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Wait-AdminLog $dir_admin "HOST\['.*'\] registered" | Out-Null
	Add-Result "control_api" "no key -> 401" ((Invoke-Http "GET" "$control/api/instances").code -eq 401)
	Add-Result "control_api" "list instances" (Wait-Http "GET" "$control/api/instances" "ctl-key" 200 '"iid":21101')
	Add-Result "control_api" "get instance (status)" (Wait-Http "GET" "$control/api/instances/21101" "ctl-key" 200 '"status":\{')
	Add-Result "control_api" "unknown instance -> 404" ((Invoke-Http "GET" "$control/api/instances/999" "ctl-key").code -eq 404)
	Add-Result "control_api" "set desired" ((Invoke-Http "PUT" "$control/api/instances/21101/desired/stopped" "ctl-key").code -eq 200)
	Add-Result "control_api" "desired is applied" (Wait-Http "GET" "$control/api/instances/21101" "ctl-key" 200 '"desired":"stopped"')
	Add-Result "control_api" "wrong method -> 405" ((Invoke-Http "GET" "$control/api/reload" "ctl-key").code -eq 405)
	Add-Result "control_api" "metrics" (Wait-Http "GET" "$control/metrics" "ctl-key" 200 "cgdk_admin_hosts 1")

	# - size limit (body 64KB / header 16KB : disconnected)
	$file_big = Join-Path $dir_admin "big_body.txt"
	[System.IO.File]::WriteAllText($file_big, ("x" * (100 * 1024)))
	$code_big_body = (& curl.exe -s -m 5 -o NUL -w "%{http_code}" -X POST -H "X-CGDK-Key: ctl-key" --data-binary "@$file_big" "$control/api/reload" 2>$null | Out-String).Trim()
	Add-Result "control_api" "too large body is refused" ($code_big_body -ne "200")
	$code_big_header = (& curl.exe -s -m 5 -o NUL -w "%{http_code}" -H "X-CGDK-Key: ctl-key" -H ("X-Big: " + ("y" * (20 * 1024))) "$control/api/instances" 2>$null | Out-String).Trim()
	Add-Result "control_api" "too large header is refused" ($code_big_header -ne "200")
	Add-Result "control_api" "server alive after large requests" ((Invoke-Http "GET" "$control/api/instances" "ctl-key").code -eq 200)

	# - signed request (deploy/tools/cgdk_control.ps1) / replayed signature
	$tool = Join-Path $root_build "src\server.admin\deploy\tools\cgdk_control.ps1"
	$env:CGDK_CONTROL_KEY = "ctl-key"; $env:CGDK_CONTROL_URL = $control; $env:CGDK_CONTROL_STATUS_ONLY = "1"
	$env:CGDK_CONTROL_TIMESTAMP = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds().ToString()
	try
	{
		$code_signed = (& powershell -ExecutionPolicy Bypass -File $tool GET /api/instances | Out-String).Trim()
		$code_replay = (& powershell -ExecutionPolicy Bypass -File $tool GET /api/instances | Out-String).Trim()
	}
	finally
	{
		foreach ($name in @("CGDK_CONTROL_KEY", "CGDK_CONTROL_URL", "CGDK_CONTROL_STATUS_ONLY", "CGDK_CONTROL_TIMESTAMP")) { [Environment]::SetEnvironmentVariable($name, $null) }
	}
	Add-Result "control_api" "signed request (hmac)" ($code_signed -eq "200")
	Add-Result "control_api" "replayed signature rejected" ($code_replay -eq "401")

	# - change the key and reload
	$file_setting = Join-Path $dir_admin "config\admin_server.setting.json"
	Write-Utf8 $file_setting ((Get-Content $file_setting -Raw) -replace "ctl-key", "ctl-key-2")
	Add-Result "control_api" "reload" ((Invoke-Http "POST" "$control/api/reload" "ctl-key").code -eq 200)
	Add-Result "control_api" "old key rejected after reload" ((Invoke-Http "GET" "$control/metrics" "ctl-key").code -eq 401)
	Add-Result "control_api" "new key accepted after reload" ((Invoke-Http "GET" "$control/metrics" "ctl-key-2").code -eq 200)

	# - audit log (json lines)
	$file_audit = Join-Path $dir_admin "log\admin_audit.jsonl"
	$text_audit = if (Test-Path $file_audit) { Get-Content $file_audit -Raw } else { "" }
	Add-Result "control_api" "audit log (json lines)" ($text_audit -match '"source":"control","address":"[^"]+","action":"POST /api/reload","result":200')
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 6) host_agent reload (server list / web ui policy)
Write-Host "6) host_reload"
$dir_admin = New-AdminDir "host_reload" '"auto_start": false' '' ''
$dir_host = New-HostAgentDir "host_reload" '' '"allowed_addresses": [ "10.255.255.1" ],'
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Wait-AdminLog $dir_admin "HOST\['.*'\] registered" | Out-Null
	Add-Result "host_reload" "web ui rejects not allowed address" ((Invoke-Http "GET" "http://127.0.0.1:$port_web/").code -eq 0)

	# - add a service to the list (host_agent checks the files every 5 seconds)
	Start-Sleep 1
	$file_list = Join-Path $dir_host "host_agent_server_list.json"
	Write-Utf8 $file_list ((Get-Content $file_list -Raw).TrimEnd().TrimEnd("}") + ', "e2e service 2": { "iid": 21102, "rank": { "group": 0, "field": 0, "role": 10, "index": 2 }, "execute_file": "not_exist_e2e_service.exe", "path": "./", "working_path": "./" } }')
	Add-Result "host_reload" "added service is registered to admin" (Wait-Http "GET" "$control/api/instances" "" 200 '"iid":21102' 20)

	# - allow loopback for the web ui
	$file_setting = Join-Path $dir_host "host_agent_setting.json"
	Write-Utf8 $file_setting ((Get-Content $file_setting -Raw) -replace "10.255.255.1", "127.0.0.1")
	Add-Result "host_reload" "web ui allowed after reload" (Wait-Http "GET" "http://127.0.0.1:$port_web/" "" 200 "" 20)
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 7) security (websocket origin / dns rebinding / auth lockout / host status)
Write-Host "7) security"
$dir_admin = New-AdminDir "security" '"auto_start": false' '' '' '"certify_key_file": "config/control.key", "auth_fail_max": 3, "auth_fail_window": 60, "auth_block_time": 60, "request_timeout": 2,'
Write-Utf8 (Join-Path $dir_admin "config\control.key") "sec-key`r`n"		# - key file (certify_key_file)
$dir_host = New-HostAgentDir "security" '' '"request_timeout": 2,' '"ping_interval": 1,'
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Wait-AdminLog $dir_admin "HOST\['.*'\] registered" | Out-Null

	# - websocket : same host web ui origin -> 101, other site -> closed
	Add-Result "security" "websocket same origin accepted" ((Invoke-WebSocket "http://127.0.0.1:$port_web") -eq 101)
	Add-Result "security" "websocket other origin rejected" ((Invoke-WebSocket "http://evil.example.com") -ne 101)
	Add-Result "security" "web ui rejects dns rebinding host" ((Invoke-Http "GET" "http://127.0.0.1:$port_web/" "" "evil.example.com:$port_web").code -eq 403)

	# - slow request (request_timeout 2s) : the connection is closed
	Add-Result "security" "slow request closed (control)" (Test-SlowRequestClosed $port_control)
	Add-Result "security" "slow request closed (web ui)" (Test-SlowRequestClosed $port_web)

	# - websocket keepalive (ping_interval 1s)
	Add-Result "security" "websocket keepalive (ping, no pong -> closed)" (Test-WebSocketKeepalive "http://127.0.0.1:$port_web")

	# - host status
	Add-Result "security" "host status (/api/hosts)" (Wait-Http "GET" "$control/api/hosts" "sec-key" 200 '"status":\{"cpu_usage"' 20)
	Add-Result "security" "host metrics" (Wait-Http "GET" "$control/metrics" "sec-key" 200 "cgdk_host_memory_used_bytes\{")
	Add-Result "security" "control timeout metrics" (Wait-Http "GET" "$control/metrics" "sec-key" 200 'cgdk_admin_control_timeouts_total\{reason="request"\} [1-9]')
	Add-Result "security" "host web metrics" (Wait-Http "GET" "$control/metrics" "sec-key" 200 'cgdk_host_web_timeouts_total\{[^}]*acceptor="web",reason="request"\} [1-9]' 20)

	# - auth lockout (3 failures -> 429)
	for ($i = 0; $i -lt 3; ++$i) { Invoke-Http "GET" "$control/metrics" "wrong-key" | Out-Null }
	Add-Result "security" "auth lockout (429)" ((Invoke-Http "GET" "$control/metrics" "sec-key").code -eq 429)
	Add-Result "security" "auth lockout is logged" (Wait-AdminLog $dir_admin "blocked: too many failures")
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 8) web ui login (host_agent "login_key")
Write-Host "8) web_login"
$dir_events = Join-Path $dir_work "web_login\events"
New-Item -ItemType Directory $dir_events -Force | Out-Null
$file_events = Join-Path $dir_events "events.txt"
$cmd_event = Join-Path $dir_events "event.cmd"
Write-Utf8 $cmd_event ("@echo %*>> `"$file_events`"`r`n")
$dir_admin = New-AdminDir "web_login" '"auto_start": false' '' '' '' ('"event_command": "' + $cmd_event.Replace('\', '\\') + '",')
$dir_host = New-HostAgentDir "web_login" '' '"login_key": "web-key",'
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Wait-AdminLog $dir_admin "HOST\['.*'\] registered" | Out-Null
	$web = "http://127.0.0.1:$port_web"

	Add-Result "web_login" "login page without session" (Wait-Http "GET" "$web/" "" 200 "LOGIN KEY")
	Add-Result "web_login" "page needs session (401)" ((Invoke-Http "GET" "$web/webpage_host_info.htm").code -eq 401)
	Add-Result "web_login" "websocket needs session" ((Invoke-WebSocket $web) -ne 101)

	$headers = (& curl.exe -s -m 5 -o NUL -D - -X POST --data "key=wrong" "$web/login" | Out-String)
	Add-Result "web_login" "wrong key -> error" ($headers -match "Location: /\?error=1")

	$headers = (& curl.exe -s -m 5 -o NUL -D - -X POST --data "key=web-key" "$web/login" | Out-String)
	$cookie = if ($headers -match "Set-Cookie: (cgdk_session=[0-9a-f]{64})") { $Matches[1] } else { "" }
	Add-Result "web_login" "login sets session cookie" ($cookie -ne "")

	$page = (& curl.exe -s -m 5 -H "Cookie: $cookie" "$web/" | Out-String)
	Add-Result "web_login" "frame page with session" ($page -match "wsUri")

	$file_audit_host = Join-Path $dir_host "log\host_agent_audit.jsonl"
	$text_audit_host = if (Test-Path $file_audit_host) { Get-Content $file_audit_host -Raw } else { "" }
	Add-Result "web_login" "host agent audit log (login)" (($text_audit_host -match '"action":"login","result":401') -and ($text_audit_host -match '"action":"login","result":200'))
	Add-Result "web_login" "websocket with session" ((Invoke-WebSocket $web $cookie) -eq 101)

	# - event command (host_connected)
	$deadline = (Get-Date).AddSeconds(15)
	$is_event = $false
	while ((Get-Date) -lt $deadline -and $is_event -eq $false)
	{
		if ((Test-Path $file_events) -and ((Get-Content $file_events -Raw) -match "host_connected")) { $is_event = $true } else { Start-Sleep -Milliseconds 200 }
	}
	Add-Result "web_login" "event command (host_connected)" $is_event
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

# 9) admin_server restart (host_agent reconnects and registers again, the host status is restored)
Write-Host "9) admin_restart"
$dir_admin = New-AdminDir "admin_restart" '"auto_start": false' '' ''
$dir_host = New-HostAgentDir "admin_restart" ''
$p_admin = Start-Server $exe_admin $dir_admin
Start-Sleep 2
$p_host = Start-Server $exe_host_agent $dir_host
try
{
	Add-Result "admin_restart" "first registration" (Wait-AdminLog $dir_admin "> HOST\['.*'\] registered")
	Stop-Server $p_admin
	Start-Sleep 3
	Add-Result "admin_restart" "host agent keeps running" ($p_host.HasExited -eq $false)
	$p_admin = Start-Server $exe_admin $dir_admin
	Add-Result "admin_restart" "registered again (reconnect)" (Wait-AdminLog $dir_admin "> HOST\['.*'\] registered" 40 2)
	Add-Result "admin_restart" "host status after restart" (Wait-Http "GET" "$control/api/hosts" "" 200 '"status":\{"cpu_usage"' 20)
}
finally { Stop-Server $p_host; Stop-Server $p_admin }

#-----------------------------------------------------------------------------
# result
#-----------------------------------------------------------------------------
$failed = @($script:results | Where-Object { $_.result -eq "FAIL" }).Count
Write-Host ""
Write-Host ("e2e: {0} checks, {1} failed (logs: {2})" -f $script:results.Count, $failed, $dir_work)
exit $(if ($failed -eq 0) { 0 } else { 1 })
