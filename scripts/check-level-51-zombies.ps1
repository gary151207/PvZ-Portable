$ErrorActionPreference = 'Stop'

$board = Get-Content -Raw -LiteralPath 'src/Lawn/Board.cpp'
$pattern = 'theLevel == 51 \|\| theLevel == 52[\s\S]{0,220}ZOMBIE_NORMAL[\s\S]{0,220}ZOMBIE_TRAFFIC_CONE[\s\S]{0,220}ZOMBIE_BUNGEE[\s\S]{0,220}ZOMBIE_PEA_HEAD'
if ($board -notmatch $pattern) {
    throw 'Level 6-1 must restrict spawning to normal, cone, bungee, and pea-head zombies.'
}
$pattern = 'theLevel == 53 \|\| theLevel == 54[\s\S]{0,260}ZOMBIE_NORMAL[\s\S]{0,260}ZOMBIE_TRAFFIC_CONE[\s\S]{0,260}ZOMBIE_BUNGEE[\s\S]{0,260}ZOMBIE_PEA_HEAD[\s\S]{0,260}ZOMBIE_NEWSPAPER'
if ($board -notmatch $pattern) {
    throw 'Level 6-3 and 6-4 must add newspaper zombies to the first pool.'
}
$pattern = 'theLevel >= 55 && theLevel <= 59[\s\S]{0,300}ZOMBIE_CATAPULT[\s\S]{0,300}ZOMBIE_ZAMBONI[\s\S]{0,300}ZOMBIE_GARGANTUAR[\s\S]{0,300}ZOMBIE_FOOTBALL[\s\S]{0,300}ZOMBIE_DANCER'
if ($board -notmatch $pattern) {
    throw 'Level 6-6 through 6-9 must add catapult, zamboni, gargantuar, football, and dancer zombies.'
}

Write-Output 'Level 6-1 zombie pool checks passed.'
