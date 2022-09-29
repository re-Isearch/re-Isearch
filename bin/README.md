This directory contains some pre-compiled binaries for re-Isearch.


<PRE>
$ md5sum *
eaf6ec5023733722c68aff02ef0fff42  Idelete
ba8283434743d1cb5a5820dd511ef658  Iindex
c6f932f907bf4dd19aac898940d1dbc7  Isearch
30ad415d5e13680a6bf5110065eea744  Iutil
c8f3d2e67045a239d8d56eec55754659  Iwatch



$ ldd *
Idelete:
	linux-vdso.so.1 (0x00007ff6c7bd9000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007ff6c74f3000)
	libibSearch.so => /media/edz/T7/github/re-Isearch/lib/libibSearch.so (0x00007ff6c71a3000)
	libibDoc.so => /media/edz/T7/github/re-Isearch/lib/libibDoc.so (0x00007ff6c6e8f000)
	libibProcess.so => /media/edz/T7/github/re-Isearch/lib/libibProcess.so (0x00007ff6c6c8b000)
	libibIO.so => /media/edz/T7/github/re-Isearch/lib/libibIO.so (0x00007ff6c6a7e000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007ff6c687a000)
	libibApps.so => /media/edz/T7/github/re-Isearch/lib/libibApps.so (0x00007ff6c6646000)
	libnsl.so.1 => /lib/x86_64-linux-gnu/libnsl.so.1 (0x00007ff6c642c000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007ff6c6228000)
	libmagic.so.1 => /usr/lib/x86_64-linux-gnu/libmagic.so.1 (0x00007ff6c6006000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007ff6c5bf9000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007ff6c585b000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007ff6c5643000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007ff6c5252000)
	/lib64/ld-linux-x86-64.so.2 (0x00007ff6c79b2000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007ff6c5035000)
Iindex:
	linux-vdso.so.1 (0x00007ffdf210f000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007fe150295000)
	libibSearch.so => /media/edz/T7/github/re-Isearch/lib/libibSearch.so (0x00007fe14ff45000)
	libibDoc.so => /media/edz/T7/github/re-Isearch/lib/libibDoc.so (0x00007fe14fc31000)
	libibProcess.so => /media/edz/T7/github/re-Isearch/lib/libibProcess.so (0x00007fe14fa2d000)
	libibIO.so => /media/edz/T7/github/re-Isearch/lib/libibIO.so (0x00007fe14f820000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007fe14f61c000)
	libibApps.so => /media/edz/T7/github/re-Isearch/lib/libibApps.so (0x00007fe14f3e8000)
	libnsl.so.1 => /lib/x86_64-linux-gnu/libnsl.so.1 (0x00007fe14f1ce000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007fe14efca000)
	libmagic.so.1 => /usr/lib/x86_64-linux-gnu/libmagic.so.1 (0x00007fe14eda8000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007fe14e99b000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fe14e5fd000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007fe14e3e5000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fe14dff4000)
	/lib64/ld-linux-x86-64.so.2 (0x00007fe15074a000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007fe14ddd7000)
Isearch:
	linux-vdso.so.1 (0x00007ffc8adda000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007f54afcef000)
	libibSearch.so => /media/edz/T7/github/re-Isearch/lib/libibSearch.so (0x00007f54af99f000)
	libibDoc.so => /media/edz/T7/github/re-Isearch/lib/libibDoc.so (0x00007f54af68b000)
	libibProcess.so => /media/edz/T7/github/re-Isearch/lib/libibProcess.so (0x00007f54af487000)
	libibIO.so => /media/edz/T7/github/re-Isearch/lib/libibIO.so (0x00007f54af27a000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007f54af076000)
	libibApps.so => /media/edz/T7/github/re-Isearch/lib/libibApps.so (0x00007f54aee42000)
	libnsl.so.1 => /lib/x86_64-linux-gnu/libnsl.so.1 (0x00007f54aec28000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f54aea24000)
	libmagic.so.1 => /usr/lib/x86_64-linux-gnu/libmagic.so.1 (0x00007f54ae802000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f54ae3f5000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f54ae057000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f54ade3f000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f54ada4e000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f54b01a4000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f54ad831000)
Iutil:
	linux-vdso.so.1 (0x00007ffd5c7f4000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007f76cf3a2000)
	libibSearch.so => /media/edz/T7/github/re-Isearch/lib/libibSearch.so (0x00007f76cf052000)
	libibDoc.so => /media/edz/T7/github/re-Isearch/lib/libibDoc.so (0x00007f76ced3e000)
	libibProcess.so => /media/edz/T7/github/re-Isearch/lib/libibProcess.so (0x00007f76ceb3a000)
	libibIO.so => /media/edz/T7/github/re-Isearch/lib/libibIO.so (0x00007f76ce92d000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007f76ce729000)
	libibApps.so => /media/edz/T7/github/re-Isearch/lib/libibApps.so (0x00007f76ce4f5000)
	libnsl.so.1 => /lib/x86_64-linux-gnu/libnsl.so.1 (0x00007f76ce2db000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f76ce0d7000)
	libmagic.so.1 => /usr/lib/x86_64-linux-gnu/libmagic.so.1 (0x00007f76cdeb5000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f76cdaa8000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f76cd70a000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f76cd4f2000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f76cd101000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f76cf857000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f76ccee4000)
Iwatch:
	linux-vdso.so.1 (0x00007ffdcbffd000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007ff61fbd5000)
	/lib64/ld-linux-x86-64.so.2 (0x00007ff6201c9000)

</PRE>



This project was funded through the NGI0 Discovery Fund, a fund established by NLnet with financial support from the European Commission's Next Generation Internet programme, under the aegis of DG Communications Networks, Content and Technology under grant agreement No 825322.


<IMG SRC="https://nlnet.nl/image/logo_nlnet.svg" ALT="NLnet Foundation" height=100> <IMG SRC="https://nlnet.nl/logo/NGI/NGIZero-green.hex.svg" ALT="NGI0 Search" height=100> &nbsp; &nbsp; <IMG SRC="https://ngi.eu/wp-content/uploads/sites/77/2017/10/bandiera_stelle.png" ALT="EU" height=100>

