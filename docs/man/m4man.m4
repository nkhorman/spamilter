dnl --------------------------------------------------------------------*
dnl 
dnl  Developed by;
dnl       Neal Horman - http://www.wanlink.com
dnl		Yes, I don't know how to write m4 macros...
dnl 
dnl  DESCRIPTION:
dnl       m4 macros for groff_mdoc generation of man pages
dnl 
dnl --------------------------------------------------------------------*
dnl
define(`ALPHA', `abcdefghijklmnopqrstuvwxyz')dnl
define(`ALPHA_UPR', `ABCDEFGHIJKLMNOPQRSTUVWXYZ')dnl
dnl
define(`MANheader', `.TH translit($1, ALPHA, ALPHA_UPR) $2 "$3" "$4" "$5"')dnl
define(`MANsection', `.SH "translit($1, ALPHA, ALPHA_UPR)"')dnl
define(`MANbold', `.B $1')dnl
dnl
define(`DMpage',
	`define(`DMappname', `$1')dnl
define(`DMpagesec',`$2')dnl
define(`DMappversion',`$3')dnl
define(`DMcreatedate',`$4')dnl
define(`DMauthor',`$5')dnl
define(`DMpurpose',`$6')dnl
MANheader(DMappname,DMpagesec,DMappversion,DMcreatedate)
MANsection(`name')
MANbold(DMappname)'
\- DMpurpose)dnl
dnl
define(`DMoption', `.TP
MANbold(-`'$1)
$2')dnl
dnl
define(`MANlinebreak', `.br')dnl
define(`MANunderline', `.I $1')dnl
dnl
define(`MANrs',`.RS')dnl
define(`MANre',`.RE')dnl
dnl
define(`MANip',`.IP \(bu 4
$1')dnl
dnl
