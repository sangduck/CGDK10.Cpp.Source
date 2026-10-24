#-----------------------------------------------------------------------------
# cgdk_control.ps1 : admin server control api with a request signature (HMAC-SHA256)
#
# usage)
#   powershell -File cgdk_control.ps1 <METHOD> <path> [curl options...]
#     ex) cgdk_control.ps1 GET /api/instances
#         cgdk_control.ps1 POST /api/instances/21101/start
#         cgdk_control.ps1 GET /metrics -o NUL -w "%{http_code}"
#
# environment)
#   CGDK_CONTROL_URL       : http://127.0.0.1:21888 (default)
#   CGDK_CONTROL_KEY       : "certify_key" of "acceptor_control" (required)
#   CGDK_CONTROL_BODY      : request body (default: empty)
#   CGDK_CONTROL_TIMESTAMP : unix time (default: now. for test only)
#   CGDK_CONTROL_STATUS_ONLY=1 : prints the http status only
#
# signature)
#   X-CGDK-Timestamp : unix time (seconds)
#   X-CGDK-Signature : hex(HMAC-SHA256(key, METHOD + "\n" + path + "\n" + timestamp + "\n" + hex(SHA256(body))))
#   (the key itself is not sent)
#-----------------------------------------------------------------------------
param(
	[Parameter(Mandatory = $true, Position = 0)][string]$Method,
	[Parameter(Mandatory = $true, Position = 1)][string]$Path,
	[Parameter(ValueFromRemainingArguments = $true)][string[]]$CurlArgs = @()
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrEmpty($env:CGDK_CONTROL_KEY)) { Write-Error "CGDK_CONTROL_KEY is not set"; exit 2 }

$url = if ($env:CGDK_CONTROL_URL) { $env:CGDK_CONTROL_URL } else { "http://127.0.0.1:21888" }
$body = if ($env:CGDK_CONTROL_BODY) { $env:CGDK_CONTROL_BODY } else { "" }
$timestamp = if ($env:CGDK_CONTROL_TIMESTAMP) { $env:CGDK_CONTROL_TIMESTAMP } else { [DateTimeOffset]::UtcNow.ToUnixTimeSeconds().ToString() }

$utf8 = New-Object System.Text.UTF8Encoding($false)
$sha256 = [System.Security.Cryptography.SHA256]::Create()
$body_hash = -join ($sha256.ComputeHash($utf8.GetBytes($body)) | ForEach-Object { $_.ToString("x2") })

$hmac = New-Object System.Security.Cryptography.HMACSHA256 (, $utf8.GetBytes($env:CGDK_CONTROL_KEY))
$message = "$Method`n$Path`n$timestamp`n$body_hash"
$signature = -join ($hmac.ComputeHash($utf8.GetBytes($message)) | ForEach-Object { $_.ToString("x2") })

$args_curl = @("-s", "-m", "10", "-X", $Method, "-H", "X-CGDK-Timestamp: $timestamp", "-H", "X-CGDK-Signature: $signature")
if ($body -ne "") { $args_curl += @("--data-binary", $body) }
if ($env:CGDK_CONTROL_STATUS_ONLY -eq "1") { $args_curl += @("-o", "NUL", "-w", "%{http_code}") }
$args_curl += $CurlArgs
$args_curl += "$url$Path"

& curl.exe @args_curl
exit $LASTEXITCODE
