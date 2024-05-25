# ps_pp
Jednostavan program nalik programu ps za ispisivanje osnovnih informacija o procesima na Linux sistemu.
Ovaj program razvijen je za projekat iz predmeta Arhitektura Računara i Operativni Sistemi na Fakultetu Organizacionih nauka.
## Mogućnosti
* Rad u realnom vremenu
* Ispisivanje osnovnih generalnih informacija o sistemu, poput ukupnog iskorišćenja procesora i memorije
* Ispisivanje svih mogućih informacija koje Linux kernel snabdeva o individualnim procesima
* Fleksibilno formatiranje informacija
* Interakcija sa procesima u realnom vremenu
* Sortiranje liste procesa po različitim parametrima
* Filtriranje procesa po PID, vlasniku, i imenu
* Modifikacija izgleda programa i keybinding-a kroz konfiguracioni fajl
* Jednostavan korisnički interfejs na komandnoj liniji
* Automatsko detektovanje gde je *proc* fajlsistem mount-ovan i sa kojim opcijama
## Kompajliranje
Kompilacija programa se vrsi jednostavnim pozivom iz root direktorijuma sa komandne linije:
```make```
Nakon čega će se u direktorijumu *obj* generisati objektni fajlovi, a u *bin* konačan program.
Kako bi se binary instalirao u PATH za korisnika, kao i konfiguracioni fajl zajedno sa predefinisanim temama, potrebno je pokrenuti:
```make install_user```
Dok je za instalaciju za ceo sistem (uz odgovarajuce permisije) potrebno pokrenuti:
```make install_system```
Direktorijum koji sadrzi konfiguracioni fajl nalazi se na putanji **~/.config/ps_pp.conf.d** za individualnog korisnika, odnosno **/etc/ps_pp.conf.d** za ceo sistem (konfiguracioni fajl korisnika ima prednost, ako postoji)

Za deinstalaciju potrebno je samo pokrenuti odgovarajuce ```make uninstall``` komande.


Za kompajliranje je potrebna biblioteka softverskog paketa ncurses za korisnički interfejs (**-lncurses** flag, dinamički linkovana).
Verzija koriscena za ovaj projekat je **ncurses-6.4-7** , mada bi program trebao raditi za bilo koju verziju.
Ova biblioteka bi trebalo biti već preinstalirana na većini Linux sistema.
## Uputstvo za korišćenje
Program ima ugrađen help meni u sebi, koji prikazuje opcije za pokretanje, kao i opcije tokom izvršavanja.
Podrazumevano dugme za otvaranje ovog menija je dugme ```h```
## Implementacija 
Ovaj program, kao i *ps*, funkcioniše tako što čita podatke iz virtuelnog fajl sistema **procfs** kojeg dinamički generiše kernel: https://man7.org/linux/man-pages/man5/proc.5.html
