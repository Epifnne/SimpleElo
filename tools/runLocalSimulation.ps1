Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$clientExe = Join-Path $root "Build\client\simpleelo_client.exe"
$serverExe = Join-Path $root "Build\server\simpleelo_server.exe"
$dataFile = Join-Path $root "serverData.json"

if (Test-Path $dataFile) {
  Remove-Item $dataFile -Force
}

function Invoke-Client {
  param(
    [Parameter(Mandatory = $true)]
    [hashtable]$Payload
  )

  $json = $Payload | ConvertTo-Json -Compress -Depth 8
  $raw = $json | & $clientExe sendStdin
  if ($LASTEXITCODE -ne 0) {
    throw "Client call failed: $json"
  }
  return ($raw | ConvertFrom-Json)
}

$serverProc = Start-Process -FilePath $serverExe -WorkingDirectory $root -PassThru
Start-Sleep -Milliseconds 600

try {
  $ownerEmail = "owner@example.com"
  $memberEmail = "member@example.com"

  $null = Invoke-Client @{ action = "sendCode"; email = $ownerEmail }
  $null = Invoke-Client @{ action = "sendCode"; email = $memberEmail }

  $ownerReg = Invoke-Client @{ action = "register"; email = $ownerEmail; password = "Pass!123"; verifyCode = "123456" }
  $memberReg = Invoke-Client @{ action = "register"; email = $memberEmail; password = "Pass!123"; verifyCode = "123456" }

  if ($ownerReg.code -ne 0 -or $memberReg.code -ne 0) {
    throw "Register failed"
  }

  $ownerLogin = Invoke-Client @{ action = "login"; email = $ownerEmail; password = "Pass!123" }
  $memberLogin = Invoke-Client @{ action = "login"; email = $memberEmail; password = "Pass!123" }

  $ownerToken = $ownerLogin.token
  $memberToken = $memberLogin.token

  if (-not $ownerToken -or -not $memberToken) {
    throw "Login token missing"
  }

  $createRoom = Invoke-Client @{ action = "createRoom"; token = $ownerToken; roomName = "Ranked-1" }
  $roomId = $createRoom.roomId
  if (-not $roomId) {
    throw "room create failed"
  }

  $joinRoom = Invoke-Client @{ action = "joinRoom"; token = $memberToken; roomId = $roomId }
  if ($joinRoom.code -ne 0) {
    throw "join room failed"
  }

  $null = Invoke-Client @{ action = "setPlayer"; token = $ownerToken; roomId = $roomId; team = "red"; role = "captain"; leftEarly = $false }
  $null = Invoke-Client @{ action = "setPlayer"; token = $memberToken; roomId = $roomId; team = "blue"; role = "assassin"; leftEarly = $true }

  $matchId = "match-local-1"
  $submit = Invoke-Client @{ action = "submitMatch"; token = $ownerToken; roomId = $roomId; winner = "red"; idempotencyKey = "idem-local-1"; matchId = $matchId }
  if ($submit.code -ne 0) {
    throw "submit match failed"
  }

  $idempotent = Invoke-Client @{ action = "submitMatch"; token = $ownerToken; roomId = $roomId; winner = "red"; idempotencyKey = "idem-local-1"; matchId = $matchId }
  if (-not $idempotent.idempotent) {
    throw "idempotency check failed"
  }

  $vote = Invoke-Client @{ action = "vote"; token = $ownerToken; matchId = $matchId; approve = $true }
  if ($vote.code -ne 0) {
    throw "vote failed"
  }

  $matchStatus = Invoke-Client @{ action = "getMatch"; matchId = $matchId }
  if (-not $matchStatus.match.finalized -or -not $matchStatus.match.passed) {
    throw "match should be finalized and passed"
  }

  $ownerProfile = Invoke-Client @{ action = "getProfile"; token = $ownerToken }
  $memberProfile = Invoke-Client @{ action = "getProfile"; token = $memberToken }

  if ($ownerProfile.history.Count -lt 1 -or $memberProfile.history.Count -lt 1) {
    throw "history should be recorded"
  }

  Write-Host "Simulation success"
  Write-Host ("RoomId: {0}, MatchId: {1}, Approved: {2}/{3}" -f $roomId, $matchId, $matchStatus.match.approved, $matchStatus.match.totalEffective)
  Write-Host ("Owner elo: {0}, Member elo: {1}" -f $ownerProfile.user.elo, $memberProfile.user.elo)
}
finally {
  if ($serverProc -and -not $serverProc.HasExited) {
    Stop-Process -Id $serverProc.Id -Force
  }
}
