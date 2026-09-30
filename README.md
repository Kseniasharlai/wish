# wish

Простий Unix-термінал (Wisconsin Shell), написаний мовою C.

## Що вміє
- Інтерактивний і пакетний режим (`./wish batch.txt`)
- Вбудовані команди: `exit`, `cd`, `path`
- Запуск програм через `fork` і `execv`
- Редирекція виводу: `ls > out.txt`
- Паралельні команди: `cmd1 & cmd2`

## Збірка й запуск
    gcc -Wall -Wextra -o wish wish.c
    ./wish