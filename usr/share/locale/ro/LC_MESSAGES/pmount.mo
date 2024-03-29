��    @        Y         �  >  �  S   �           5  "   S  (   v  ;   �  )   �  !     -   '  /   U  4   �  #   �  2   �  (   	     :	      Y	     z	  %   �	  %   �	  d   �	  (   K
  1   t
     �
  %   �
  -   �
        *   ;     f      �  "   �  (   �     �  ;     ;   H  !   �  1   �  +   �  !     ,   &  %   S     y     �     �  $   �  /   �  &        E  4   [     �  D   �  C   �  6   /  6   f  0   �  5   �  %     >   *  �  i  �  A  &  �  O     6  R  �  �  I  Q  W   �  )   �       %   =  1   c  7   �  +   �     �  -     3   F  @   z  (   �  5   �  ,        G     d     �  )   �  (   �  p   �  *   e  3   �  "   �  0   �  .      #   G   0   k   $   �   '   �   )   �   7   !  "   K!  9   n!  ?   �!  "   �!  0   "  *   <"  6   g"  8   �"  7   �"     #      )#     J#  2   e#  6   �#  1   �#     $  0    $     Q$  @   j$  =   �$  2   �$  :   %  5   W%  :   �%  #   �%  F   �%  �  3&  �  3*  $  �+  V   �,  M  E-             	   >          .                *      ,       0           /   ?           :      (       %       ;   "          '   5         &   6   @         7   9   !            $   #              )          =                        8          4                
       -      1                  +          2          <   3       %s --lock <device> <pid>
  Prevent further pmounts of <device> until it is unlocked again. <pid>
  specifies the process id the lock holds for. This allows to lock a device
  by several independent processes and avoids indefinite locks of crashed
  processes (nonexistant pids are cleaned before attempting a mount).

 %s --unlock <device> <pid>
  Remove the lock on <device> for process <pid> again.

 Error: %s is not a block device
 Error: %s is not a directory
 Error: '%s' is not a valid number
 Error: '/' must not occur in label name
 Error: cannot lock for pid %u, this process does not exist
 Error: could not connect to dbus: %s: %s
 Error: could not create directory Error: could not create pid lock file %s: %s
 Error: could not create stamp file in directory Error: could not decrypt device (wrong passphrase?)
 Error: could not delete mount point Error: could not determine real path of the device Error: could not drop all uid privileges Error: could not execute mount Error: could not execute pmount
 Error: could not execute umount Error: could not get status of device Error: could not get sysfs directory
 Error: could not lock the mount directory. Another pmount is probably running for this mount point.
 Error: could not open <sysfs dir>/block/ Error: could not open <sysfs dir>/block/<device>/ Error: could not open directory Error: could not open fstab-type file Error: could not remove pid lock file %s: %s
 Error: device %s does not exist
 Error: device %s is already mounted to %s
 Error: device %s is locked
 Error: device %s is not mounted
 Error: device %s is not removable
 Error: device %s was not mounted by you
 Error: device name too long
 Error: directory %s already contains a mounted file system
 Error: directory %s does not contain a mounted file system
 Error: directory %s is not empty
 Error: do_unlock: could not remove lock directory Error: given UDI is not a mountable volume
 Error: invalid charset name '%s'
 Error: invalid device %s (must be in /dev/)
 Error: invalid file system name '%s'
 Error: invalid umask %s
 Error: label must not be empty
 Error: label too long
 Error: mapped device already exists
 Error: mount point %s is already in /etc/fstab
 Error: mount point %s is not below %s
 Error: out of memory
 Error: this program needs to be installed suid root
 Error: umount failed
 Internal error: could not change effective group id to real group id Internal error: could not change effective user uid to real user id Internal error: could not change to effective gid root Internal error: could not change to effective uid root Internal error: could not determine mount point
 Internal error: getopt_long() returned unknown value
 Internal error: mode %i not handled.
 Internal error: mount_attempt: given file system name is NULL
 Options:
  -r          : force <device> to be mounted read-only
  -w          : force <device> to be mounted read-write
  -s, --sync  : mount <device> with the 'sync' option (default: 'async')
  --noatime   : mount <device> with the 'noatime' option (default: 'atime')
  -e, --exec  : mount <device> with the 'exec' option (default: 'noexec')
  -t <fs>     : mount as file system type <fs> (default: autodetected)
  -c <charset>: use given I/O character set (default: 'utf8' if called
                in an UTF-8 locale, otherwise mount default)
  -u <umask>  : use specified umask instead of the default (only for
                file sytems which actually support umask setting)
 --passphrase <file>
                read passphrase from file instead of the terminal
                (only for LUKS encrypted devices)
  -d, --debug : enable debug output (very verbose)
  -h, --help  : print help message and exit successfuly
  --version   : print version number and exit successfully Usage:

%s [options] <device>
  Umount <device> from a directory below %s if policy requirements
  are met (see pumount(1) for details). The mount point directory is removed
  afterwards.

Options:
  -l, --lazy  : umount lazily, see umount(8)
  -d, --debug : enable debug output (very verbose)
  -h, --help  : print help message and exit successfuly
  --version   : print version number and exit successfully
 Usage:

%s [options] <device> [<label>]

  Mount <device> to a directory below %s if policy requirements
  are met (see pmount(1) for details). If <label> is given, the mount point
  will be %s/<label>, otherwise it will be %s<device>.
  If the mount point does not exist, it will be created.

 Warning: device %s is already handled by /etc/fstab, supplied label is ignored
 pmount-hal - execute pmount with additional information from hal

Usage: pmount-hal <device> [pmount options]

This command mounts the device described by the given device or UDI using
pmount. The file system type, the volume storage policy and the desired label
will be read out from hal and passed to pmount. Project-Id-Version: mount removable devices as normal user
Report-Msgid-Bugs-To: martin.pitt@canonical.com
POT-Creation-Date: 2006-08-15 23:38+0200
PO-Revision-Date: 2007-12-11 20:30+0000
Last-Translator: fork <Unknown>
Language-Team: Romanian <ro@li.org>
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
X-Launchpad-Export-Date: 2010-05-04 20:48+0000
X-Generator: Launchpad (build Unknown)
X-Rosetta-Version: 0.1
 %s --lock <dispozitiv> <pid>
Previne pmounts-uri viitoare ale <dispozitiv> pana cand este deblocat.
<pid> specifica id-ul procesului mentinut de lock.Aceasta pemite blocarea unui dispozitiv
prin cateva procese independente si evita blocari indefinite
ale proceselor blocate (pid-uri inexistente sunt sterse inainte de montare).

 %s --unlock <dispozitive> <pid>
Sterge lock-ul pe <dispozitiv> pentru procesul <pid>.

 Eroare: %s nu e un dispozitiv de blocuri
 Eroare: %s nu este un director
 Eroare: '%s' nu este un număr valid
 Eroare:'/' nu trebuie sa apara in numele <label>
 Eroare: nu pot bloca pid-ul %u, acest proces nu exista
 Eroare: nu mă pot conecta la dbus: %s: %s
 Eroare: nu pot crea directorul Eroare: nu pot crea fisierul pid lock %s: %s
 Eroare: nu am putut crea fisierul stamp in director Eroare: nu am putut decripta dispozitivul (passphrase gresita?)
 Eroare: nu pot sterge punctul de montare Eroare: nu pot determina calea reala a dispozitivului Eroare: nu pot scadea toate privilegiile uid Eroare: nu pot executa mount Eroare: nu pot executa pmount
 Eroare: nu pot executa umount Eroare: nu pot afla starea dispozitivului Eroare: nu pot obţine directorul sysfs
 Eroare: nu s-a putut bloca directorul. Probabil exista deja lansat un alt pmount pentru acest punct de montare.
 Eroare: nu pot deschide <sysfs dir>/block/ Eroare: nu pot deschide <sysfs dir>/block/<device>/ Eroare: nu pot deschide directorul Eroare: nu am putut deschide fisierul fstab-type Eroare: nu pot sterge fisierul pid lock %s:%s
 Eroare: dispozitivul %s nu există
 Eroare: dispozitivul %s este deja montat în %s
 Eroare: dispozitivul %s este blocat
 Eroare: dispozitivul %s nu este montat
 Eroare: dispozitivul %s nu este amovibil
 Eroare: dispozitivul %s nu a fost montat de catre tine
 Eroare: nume dispozitiv prea lung
 Eroare: directorul %s contine deja un file system montat
 Eroare: directorul %s nu conţine un sistem de fişiere montat
 Eroare: directorul %s nu este gol
 Eroare: do unlocl: nu mot sterge directorul lock Eroare: UDI-ul dat nu e un volum montabil
 Eroare: numele '%s' setului de caractere este invalid
 Eroare: dispozitiv %s invalid (necesar sa fie in /dev/)
 Eroare: numele '%s' sistemului de fisiere este invalid
 Eroare: umask invalid %s
 Eroare: <label> nu poate fi gol
 Eroare: <label> prea lung
 Eroare: Exista deja un despozitiv cartat (mapped)
 Eroare: punctul de mount %s exista deja in /etc/fstab
 Eroare: punctul de montare %s nu se află sub %s
 Eroare: memorie insuficientă
 Eroare: programul trebuie instalat cu suid root
 Eroare: umount a eşuat
 Eroare interna: nu pot schimba group id efectiv in group id real Eroare interna: nu pot schimba uid efectiv in user id-ul real Eroare interna: nu pot schimba in root gid efectiv Eroare interna: nu am putut schimba in uid-ul root efectiv Eroare internă: nu pot determina punctul de montare
 Eroare interna: getopt_logn() a intor valoare necunoscuta
 Eroare interna: mod %i nesuportat.
 Eroare interna: mount_attempt: numele sistemului de fisiere este NULL
 Optiuni:
  -r : forteaza <dispozitivul> sa fie montat read-only
  -w : forteaza <dispozitivul> sa fie montat read-write
  -s, --sync : monteaza <dispozitivul> cu optiunea 'sync' (standard: 'async')
  --noatime : monteaza <dispozitivul> cu optiunea 'noatime' (standard: 'atime')
  -e, --exec : monteaza <dispozitivul> cu optiunea 'exec' (standard: 'noexec')
  -t <fs> : monteaza ca tip de sistem de fisiere <fs> (standard: autodetected)
  -c <charset>: foloseste setul de caractere I/O dat (default: 'utf8' daca e cheamat
                UTF-8 locale, altfel se monteaza standard)
  -u <umask> : foloseste umask specificat in locul celui standard (doar pentru
                sisteme de fisiere care accepta setari umask)
 --passphrase <file>
                citeste passphrase-ul din fisier in loc de terminal
                (doar pentru dispozitive encriptate LUKS)
  -d, --debug : porneste afisarea debug (foarte verbose)
  -h, --help : arata mesajul de ajutor si inchide
  --version : arata numarul versiunii si inchide Folosire:

%s [optiuni] <dispozitiv>
  Demonteaza <dispozitiv> dintr-un director sub %s daca cererile sunt
  satisfacute (vezi pumount(1) pentru detalii). Directorul punct de
  montare este apoi sters.

Optiuni:
-l, --lazy : demonteaza lent, vezi umount(8)
-d, --debug : porneste afisarea debug (foarte verbose)
-h, --help : arata mesajul de ajutor apoi iese
--version : arata numarul versiunii apoi iese
 Folosire:
%s [optiuni]<dispozitiv>[<label>]

  Mount <dispozitiv> intr-un directorsub %s daca cerintele
sunt intalnite(vezi pmount(1) pentru detalii). Daca ,label> este dat, punctul de montare va fi %s/<label>, altfel va fi %s<dispozitiv>.
Daca punctul de montare nu exista, el va fi creat.

 Atentie: dispozitivul %s este folosti de /etc/fstab, eticheta furnizata este ignorata
 pmount-hal - executa pmount cu informatii suplimentare de la hal

Folosire: pmount-hal <dispozitiv> [optiuni pmount]

Aceasta comanda monteaza dispozitivul in functie de dispozitivul dat sau de UDI
folosind pmount. Sistemul de fisiere, politica de depozitare a volumului si eticheta
dorita vor fi citite fin hal si pasate lui pmount. 