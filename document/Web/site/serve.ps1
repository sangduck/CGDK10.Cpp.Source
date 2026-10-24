param([int]$Port = $(if ($env:PORT) { [int]$env:PORT } else { 8089 }),[string]$Root = "D:\CGDK\CGDK10.Cpp\document\Web\site")
$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://localhost:$Port/")
$listener.Start()
Write-Host "serving $Root on http://localhost:$Port/"
$types = @{ ".html"="text/html; charset=utf-8"; ".css"="text/css; charset=utf-8"; ".js"="text/javascript"; ".png"="image/png";".jpg"="image/jpeg"; ".svg"="image/svg+xml"; ".ico"="image/x-icon" }
while ($listener.IsListening) {
  $ctx = $listener.GetContext()
  $path = [uri]::UnescapeDataString($ctx.Request.Url.AbsolutePath)
  if ($path -eq "/") { $path = "/index.html" }
  $file = Join-Path $Root ($path -replace "/", "\")
  try {
    if (Test-Path $file -PathType Leaf) {
      $bytes = [IO.File]::ReadAllBytes($file)
      $ext = [IO.Path]::GetExtension($file).ToLower()
      $ctx.Response.ContentType = if ($types.ContainsKey($ext)) { $types[$ext] } else { "application/octet-stream" }
      $ctx.Response.StatusCode = 200
      $ctx.Response.OutputStream.Write($bytes, 0, $bytes.Length)
    } else {
      $ctx.Response.StatusCode = 404
    }
  } catch { $ctx.Response.StatusCode = 500 }
  $ctx.Response.Close()
}
