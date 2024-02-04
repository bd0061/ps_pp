# ps_pp
Jednostavan program nalik programu ps za ispisivanje osnovnih informacija o procesima na Linux sistemu.
Ovaj program razvijen je za projekat iz predmeta Arhitektura Računara i Operativni Sistemi na Fakultetu Organizacionih nauka.
## Mogućnosti
* Rad u realnom vremenu
* Ispisivanje osnovnih generalnih informacija o sistemu, poput iskorišćenja procesora i memorije
* Ispisivanje svih mogućih informacija koje Linux kernel snabdeva o individualnim procesima
* Fleksibilno formatiranje informacija
* Interakcija sa procesima u realnom vremenu
* Sortiranje liste procesa
* Filtriranje procesa po PID, vlasniku, i imenu
* Modifikacija izgleda programa i keybinding-a kroz konfiguracioni fajl
* Jednostavan korisnički interfejs na komandnoj liniji
## Kompajliranje
Kompilacija programa se vrsi jednostavnim pozivom iz root direktorijuma sa komandne linije:
```make```
Nakon čega će se u direktorijumu *obj* generisati objektni fajlovi, a u *bin* konačan program.

Za kompajliranje je potrebna biblioteka posix niti (**-lpthread** flag) i softverskog paketa ncurses za korisnički interfejs (**-lncurses** flag).
Obe biblioteke bi trebalo biti već preinstalirane na većini Linux sistema.
## Implementacija 
Ovaj program, kao i *ps*, funkcioniše tako što čita podatke iz virtuelnog fajl sistema **procfs** kojeg dinamički generiše kernel: https://man7.org/linux/man-pages/man5/proc.5.html
