$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$dataPath = Join-Path $root 'serverData.json'
$backupPath = Join-Path $root ('serverData.backup.' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')

if (Test-Path $dataPath) {
  Copy-Item -Path $dataPath -Destination $backupPath -Force
}

# Hash value compatible with current dataset for password: Pass!123
$defaultPasswordHash = '11913469231378811836'

$users = @(
  @{ userId = 1; email = 'owner@example.com';   nickname = 'owner';   elo = 1032; wins = 6; losses = 4; passwordHash = $defaultPasswordHash },
  @{ userId = 2; email = 'member@example.com';  nickname = 'member';  elo = 998;  wins = 3; losses = 6; passwordHash = $defaultPasswordHash },
  @{ userId = 3; email = 'player@example.com';  nickname = 'player';  elo = 1015; wins = 5; losses = 5; passwordHash = $defaultPasswordHash },
  @{ userId = 4; email = 'alpha@example.com';   nickname = 'alpha';   elo = 1048; wins = 7; losses = 3; passwordHash = $defaultPasswordHash },
  @{ userId = 5; email = 'bravo@example.com';   nickname = 'bravo';   elo = 988;  wins = 2; losses = 7; passwordHash = $defaultPasswordHash },
  @{ userId = 6; email = 'charlie@example.com'; nickname = 'charlie'; elo = 1024; wins = 6; losses = 4; passwordHash = $defaultPasswordHash },
  @{ userId = 7; email = 'delta@example.com';   nickname = 'delta';   elo = 970;  wins = 1; losses = 8; passwordHash = $defaultPasswordHash },
  @{ userId = 8; email = 'echo@example.com';    nickname = 'echo';    elo = 1062; wins = 8; losses = 2; passwordHash = $defaultPasswordHash },
  @{ userId = 9; email = 'foxtrot@example.com'; nickname = 'foxtrot'; elo = 1006; wins = 4; losses = 5; passwordHash = $defaultPasswordHash },
  @{ userId = 10; email = 'golf@example.com';   nickname = 'golf';    elo = 993;  wins = 3; losses = 6; passwordHash = $defaultPasswordHash }
)

$rooms = @(
  @{
    roomId = 'room-1'; roomName = 'Ranked-1'; roomPassword = ''; ownerUserId = 1; locked = $false; gameStarted = $false; bpEnabled = $true;
    players = @(
      @{ userId = 1; team = 'red';  role = 'captain';  roleScore = 10; ready = $true;  online = $true;  leftEarly = $false },
      @{ userId = 2; team = 'blue'; role = 'assassin'; roleScore = 8;  ready = $false; online = $true;  leftEarly = $false },
      @{ userId = 3; team = 'red';  role = 'support';  roleScore = 6;  ready = $true;  online = $true;  leftEarly = $false },
      @{ userId = 7; team = 'blue'; role = 'tank';     roleScore = 5;  ready = $false; online = $false; leftEarly = $true }
    )
  },
  @{
    roomId = 'room-2'; roomName = 'Ranked-2'; roomPassword = ''; ownerUserId = 4; locked = $false; gameStarted = $false; bpEnabled = $false;
    players = @(
      @{ userId = 4; team = 'red';  role = 'captain'; roleScore = 10; ready = $true; online = $true; leftEarly = $false },
      @{ userId = 5; team = 'blue'; role = 'adc';     roleScore = 7;  ready = $true; online = $true; leftEarly = $false },
      @{ userId = 6; team = 'red';  role = 'mid';     roleScore = 8;  ready = $true; online = $true; leftEarly = $false },
      @{ userId = 8; team = 'blue'; role = 'jungler'; roleScore = 9;  ready = $true; online = $true; leftEarly = $false }
    )
  },
  @{
    roomId = 'room-3'; roomName = 'Practice'; roomPassword = '1234'; ownerUserId = 9; locked = $false; gameStarted = $false; bpEnabled = $false;
    players = @(
      @{ userId = 9; team = 'red';  role = 'captain'; roleScore = 9; ready = $false; online = $true; leftEarly = $false },
      @{ userId = 10; team = 'blue'; role = 'support'; roleScore = 7; ready = $false; online = $true; leftEarly = $false }
    )
  }
)

$matchRows = @(
  @{ matchId='match-1001'; roomId='room-1'; ownerUserId=1; winner='red';  red=@(1,3); blue=@(2,7) },
  @{ matchId='match-1002'; roomId='room-1'; ownerUserId=1; winner='blue'; red=@(1,2); blue=@(3,6) },
  @{ matchId='match-1003'; roomId='room-2'; ownerUserId=4; winner='red';  red=@(4,6); blue=@(5,8) },
  @{ matchId='match-1004'; roomId='room-2'; ownerUserId=4; winner='blue'; red=@(4,5); blue=@(6,8) },
  @{ matchId='match-1005'; roomId='room-3'; ownerUserId=9; winner='red';  red=@(9);   blue=@(10) },
  @{ matchId='match-1006'; roomId='room-1'; ownerUserId=1; winner='red';  red=@(1,4); blue=@(2,5) },
  @{ matchId='match-1007'; roomId='room-1'; ownerUserId=1; winner='blue'; red=@(3,6); blue=@(8,9) },
  @{ matchId='match-1008'; roomId='room-2'; ownerUserId=4; winner='red';  red=@(4,8); blue=@(1,10) },
  @{ matchId='match-1009'; roomId='room-2'; ownerUserId=4; winner='blue'; red=@(2,7); blue=@(5,6) },
  @{ matchId='match-1010'; roomId='room-3'; ownerUserId=9; winner='red';  red=@(9,10); blue=@(3,4) },
  @{ matchId='match-1011'; roomId='room-1'; ownerUserId=1; winner='blue'; red=@(1,2); blue=@(8,10) },
  @{ matchId='match-1012'; roomId='room-2'; ownerUserId=4; winner='red';  red=@(4,6); blue=@(7,9) }
)

$now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
$matches = @()
$matchHistory = @()
$idx = 0
foreach ($m in $matchRows) {
  $created = $now - (1200 * ($matchRows.Count - $idx))
  $deadline = $created + 60
  $idempotencyKey = 'idem-' + $m.matchId

  $approved = @($m.red + $m.blue)
  $matches += @{
    matchId = $m.matchId
    roomId = $m.roomId
    ownerUserId = $m.ownerUserId
    winner = $m.winner
    idempotencyKey = $idempotencyKey
    createdAtEpochSec = $created
    voteDeadlineEpochSec = $deadline
    finalized = $true
    passed = $true
    approved = $approved
    rejected = @()
    effectiveVoters = $approved
  }

  foreach ($uid in $m.red) {
    $isWin = $m.winner -eq 'red'
    $delta = if ($isWin) { 16 } else { -16 }
    $after = ($users | Where-Object { $_.userId -eq $uid }).elo
    $before = $after - $delta
    $matchHistory += @{
      matchId = $m.matchId
      roomId = $m.roomId
      userId = $uid
      eloBefore = $before
      eloAfter = $after
      delta = $delta
      outcome = if ($isWin) { 'win' } else { 'lose' }
    }
  }

  foreach ($uid in $m.blue) {
    $isWin = $m.winner -eq 'blue'
    $delta = if ($isWin) { 16 } else { -16 }
    $after = ($users | Where-Object { $_.userId -eq $uid }).elo
    $before = $after - $delta
    $matchHistory += @{
      matchId = $m.matchId
      roomId = $m.roomId
      userId = $uid
      eloBefore = $before
      eloAfter = $after
      delta = $delta
      outcome = if ($isWin) { 'win' } else { 'lose' }
    }
  }

  $idx++
}

$sessions = @{}
foreach ($u in $users) {
  $sessions[('token-' + $u.userId + '-seed')] = $u.userId
}

$verifyCodes = @{}
foreach ($u in $users) {
  $verifyCodes[$u.email] = '123456'
}

$root = @{
  nextUserId = 11
  nextRoomCounter = 4
  users = $users
  verifyCodes = $verifyCodes
  sessions = $sessions
  rooms = $rooms
  deletedRooms = @()
  matches = $matches
  matchHistory = $matchHistory
}

$root | ConvertTo-Json -Depth 8 | Set-Content -Path $dataPath -Encoding UTF8
Write-Output ('Seed complete. Backup: ' + $backupPath)
Write-Output ('Users: ' + $users.Count + ', Rooms: ' + $rooms.Count + ', Matches: ' + $matches.Count + ', HistoryRows: ' + $matchHistory.Count)
