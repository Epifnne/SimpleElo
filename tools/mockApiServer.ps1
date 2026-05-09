param(
  [string]$ListenHost = "127.0.0.1",
  [int]$Port = 19090
)

$ErrorActionPreference = "Stop"

function New-JsonLine([hashtable]$obj) {
  return (($obj | ConvertTo-Json -Depth 10 -Compress) + "`n")
}

$usersById = @{
  1 = @{ userId = 1; email = "owner@example.com"; nickname = "owner"; elo = 1032; wins = 6; losses = 4 }
  2 = @{ userId = 2; email = "member@example.com"; nickname = "member"; elo = 998; wins = 3; losses = 6 }
  3 = @{ userId = 3; email = "player@example.com"; nickname = "player"; elo = 1015; wins = 5; losses = 5 }
  4 = @{ userId = 4; email = "alpha@example.com"; nickname = "alpha"; elo = 1048; wins = 7; losses = 3 }
}

$emailToUserId = @{
  "owner@example.com" = 1
  "member@example.com" = 2
  "player@example.com" = 3
  "alpha@example.com" = 4
}

$rooms = @(
  @{ roomId = "room-1"; roomName = "Mock Ranked A"; ownerUserId = 1; memberCount = 4; hasPassword = $false; locked = $false; gameStarted = $false; bpEnabled = $true },
  @{ roomId = "room-2"; roomName = "Mock Ranked B"; ownerUserId = 4; memberCount = 2; hasPassword = $true; locked = $false; gameStarted = $false; bpEnabled = $false }
)

$roomDetails = @{
  "room-1" = @{
    room = @{ roomId = "room-1"; roomName = "Mock Ranked A"; ownerUserId = 1; locked = $false; gameStarted = $false; bpEnabled = $true }
    predicted = @{ redWinDelta = 14; blueWinDelta = -14 }
    teamRed = @(
      @{ userId = 1; nickname = "owner"; elo = 1032; role = "captain"; roleScore = 10; ready = $true; online = $true; leftEarly = $false; expectedWinDelta = 14; expectedLoseDelta = -14 },
      @{ userId = 3; nickname = "player"; elo = 1015; role = "support"; roleScore = 7; ready = $false; online = $true; leftEarly = $false; expectedWinDelta = 14; expectedLoseDelta = -14 }
    )
    teamBlue = @(
      @{ userId = 2; nickname = "member"; elo = 998; role = "adc"; roleScore = 8; ready = $true; online = $true; leftEarly = $false; expectedWinDelta = -14; expectedLoseDelta = 14 },
      @{ userId = 4; nickname = "alpha"; elo = 1048; role = "jungler"; roleScore = 9; ready = $false; online = $false; leftEarly = $true; expectedWinDelta = -14; expectedLoseDelta = 14 }
    )
  }
}

$historyByUserId = @{
  1 = @(
    @{ matchId = "match-1001"; roomId = "room-1"; createdAtEpochSec = 1777461769; role = "captain"; eloBefore = 1016; eloAfter = 1032; delta = 16; outcome = "win" },
    @{ matchId = "match-1002"; roomId = "room-1"; createdAtEpochSec = 1777461969; role = "captain"; eloBefore = 1032; eloAfter = 1016; delta = -16; outcome = "lose" }
  )
  2 = @(
    @{ matchId = "match-1001"; roomId = "room-1"; createdAtEpochSec = 1777461769; role = "adc"; eloBefore = 1014; eloAfter = 998; delta = -16; outcome = "lose" }
  )
  3 = @(
    @{ matchId = "match-1001"; roomId = "room-1"; createdAtEpochSec = 1777461769; role = "support"; eloBefore = 999; eloAfter = 1015; delta = 16; outcome = "win" }
  )
  4 = @(
    @{ matchId = "match-1002"; roomId = "room-1"; createdAtEpochSec = 1777461969; role = "jungler"; eloBefore = 1032; eloAfter = 1048; delta = 16; outcome = "win" }
  )
}

$matchById = @{
  "match-1001" = @{
    matchId = "match-1001"
    roomId = "room-1"
    winner = "red"
    createdAtEpochSec = 1777461769
    approved = 3
    rejected = 1
    totalEffective = 4
    deadlineEpochSec = 1777461829
    finalized = $true
    passed = $true
    teamRed = @(
      @{ userId = 1; nickname = "owner"; team = "red"; role = "captain"; roleScore = 10; online = $true; leftEarly = $false },
      @{ userId = 3; nickname = "player"; team = "red"; role = "support"; roleScore = 7; online = $true; leftEarly = $false }
    )
    teamBlue = @(
      @{ userId = 2; nickname = "member"; team = "blue"; role = "adc"; roleScore = 8; online = $true; leftEarly = $false },
      @{ userId = 4; nickname = "alpha"; team = "blue"; role = "jungler"; roleScore = 9; online = $false; leftEarly = $true }
    )
  }
}

function Get-UserFromToken([string]$token) {
  if ([string]::IsNullOrWhiteSpace($token)) { return $null }
  if ($token -like "mock-token-*") {
    $idText = $token.Substring("mock-token-".Length)
    $id = 0
    if ([int]::TryParse($idText, [ref]$id) -and $usersById.ContainsKey($id)) {
      return $usersById[$id]
    }
  }
  return $null
}

function Handle-Request([hashtable]$req) {
  $action = [string]$req.action
  switch ($action) {
    "ping" {
      return @{ code = 0; message = "pong"; serverTime = (Get-Date).ToString("s") }
    }
    "sendCode" {
      return @{ code = 0; message = "ok"; verifyCode = "123456"; note = "mock fixed code" }
    }
    "register" {
      $email = [string]$req.email
      if ([string]::IsNullOrWhiteSpace($email)) { return @{ code = 401; message = "email required" } }
      return @{ code = 0; message = "ok"; user = @{ userId = 99; email = $email; nickname = ([string]$req.nickname); elo = 1000; wins = 0; losses = 0 } }
    }
    "login" {
      $email = [string]$req.email
      if (-not $emailToUserId.ContainsKey($email)) {
        return @{ code = 411; message = "account not found (mock)" }
      }
      $uid = $emailToUserId[$email]
      return @{ code = 0; message = "ok"; token = "mock-token-$uid"; user = $usersById[$uid] }
    }
    "resetPassword" {
      return @{ code = 0; message = "ok" }
    }
    "listRooms" {
      return @{ code = 0; message = "ok"; rooms = $rooms }
    }
    "getRoomDetail" {
      $roomId = [string]$req.roomId
      if ($roomDetails.ContainsKey($roomId)) {
        $resp = @{ code = 0; message = "ok" }
        foreach ($kv in $roomDetails[$roomId].GetEnumerator()) {
          $resp[$kv.Key] = $kv.Value
        }
        return $resp
      }
      return @{ code = 437; message = "room not found" }
    }
    "getProfile" {
      $u = Get-UserFromToken([string]$req.token)
      if ($null -eq $u) { return @{ code = 481; message = "invalid token" } }
      $hist = @()
      if ($historyByUserId.ContainsKey($u.userId)) { $hist = $historyByUserId[$u.userId] }
      return @{ code = 0; message = "ok"; user = $u; history = $hist }
    }
    "getUserHistory" {
      $uid = [int]$req.userId
      if (-not $usersById.ContainsKey($uid)) { return @{ code = 487; message = "target user not found" } }
      $hist = @()
      if ($historyByUserId.ContainsKey($uid)) { $hist = $historyByUserId[$uid] }
      return @{ code = 0; message = "ok"; user = $usersById[$uid]; history = $hist }
    }
    "getMatch" {
      $matchId = [string]$req.matchId
      if ($matchById.ContainsKey($matchId)) {
        return @{ code = 0; message = "ok"; match = $matchById[$matchId] }
      }
      return @{ code = 471; message = "match not found" }
    }
    "createRoom" {
      return @{ code = 0; message = "ok"; roomId = "room-mock-new"; roomName = "Mock Created" }
    }
    "joinRoom" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); memberCount = 5 }
    }
    "leaveRoom" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); leftAsOffline = $false }
    }
    "setPlayer" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); team = ([string]$req.team); role = ([string]$req.role); roleScore = 8; ready = [bool]$req.ready; online = $true; leftEarly = $false }
    }
    "setRoomBp" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); bpEnabled = [bool]$req.bpEnabled }
    }
    "startGame" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); locked = $true; gameStarted = $true }
    }
    "abortGame" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); locked = $false; gameStarted = $false }
    }
    "deleteRoom" {
      return @{ code = 0; message = "ok"; roomId = ([string]$req.roomId); deleted = $true }
    }
    "submitMatch" {
      return @{ code = 0; message = "ok"; match = $matchById["match-1001"] }
    }
    "vote" {
      return @{ code = 0; message = "ok"; match = $matchById["match-1001"] }
    }
    default {
      return @{ code = 499; message = "unknown action (mock)" }
    }
  }
}

$listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse($ListenHost), $Port)
$listener.Start()
Write-Output "Mock API listening on $ListenHost`:$Port"
Write-Output "Protocol: one-line JSON request, one-line JSON response"

try {
  while ($true) {
    $client = $listener.AcceptTcpClient()
    try {
      $stream = $client.GetStream()
      $reader = New-Object System.IO.StreamReader($stream)
      $writer = New-Object System.IO.StreamWriter($stream)
      $writer.AutoFlush = $true
      $line = $reader.ReadLine()

      if ([string]::IsNullOrWhiteSpace($line)) {
        $writer.Write((New-JsonLine @{ code = 500; message = "empty request" }))
        continue
      }

      try {
        $reqObj = ConvertFrom-Json -InputObject $line
        $reqHash = @{}
        foreach ($p in $reqObj.PSObject.Properties) {
          $reqHash[$p.Name] = $p.Value
        }
      } catch {
        $writer.Write((New-JsonLine @{ code = 500; message = "invalid json" }))
        continue
      }

      $resp = Handle-Request $reqHash
      $writer.Write((New-JsonLine $resp))
    } finally {
      $client.Close()
    }
  }
}
finally {
  $listener.Stop()
}
