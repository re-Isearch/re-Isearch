This directory contains the shared libraries for 64-bit x86 Linux (Ubuntu LTS)

<PRE>

lrwxrwxrwx  1 edz edz       21 Sep 25 12:38 libibApps.so -> ../lib/libibApps.so.1
-rwxrwxr-x  1 edz edz   810952 Sep 25 12:38 libibApps.so.1
lrwxrwxrwx  1 edz edz       22 Sep 25 12:38 libibDoc.so -> ../lib/libibDoc.so.4.7
-rwxrwxr-x  1 edz edz  5625992 Sep 25 12:38 libibDoc.so.4.7
lrwxrwxrwx  1 edz edz       21 Sep 25 12:26 libibIO.so -> ../lib/libibIO.so.4.7
-rwxrwxr-x  1 edz edz   256232 Sep 25 12:26 libibIO.so.4.7
lrwxrwxrwx  1 edz edz       22 Sep 25 12:38 libibLocal.so -> ../lib/libibLocal.so.1
-rwxrwxr-x  1 edz edz    19256 Sep 25 12:38 libibLocal.so.1
lrwxrwxrwx  1 edz edz       26 Aug 17  2021 libibProcess.so -> ../lib/libibProcess.so.4.7
-rwxrwxr-x  1 edz edz    51688 Aug 17  2021 libibProcess.so.4.7
lrwxrwxrwx  1 edz edz       25 Sep 25 12:38 libibSearch.so -> ../lib/libibSearch.so.4.7
-rwxrwxr-x  1 edz edz  5801464 Sep 25 12:38 libibSearch.so.4.7
-rw-rw-r--  1 edz edz 13445192 Aug 16  2021 libibSearch.so.profile
lrwxrwxrwx  1 edz edz       24 Sep 25 12:38 libibUtils.so -> ../lib/libibUtils.so.4.7
-rwxrwxr-x  1 edz edz  1987320 Sep 25 12:38 libibUtils.so.4.7


$ ldd *.so
libibApps.so:
	linux-vdso.so.1 (0x00007ffe28181000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007fc4d649c000)
	libibSearch.so => /media/edz/T7/github/re-Isearch/lib/libibSearch.so (0x00007fc4d614c000)
	libibDoc.so => /media/edz/T7/github/re-Isearch/lib/libibDoc.so (0x00007fc4d5e38000)
	libibProcess.so => /media/edz/T7/github/re-Isearch/lib/libibProcess.so (0x00007fc4d5c34000)
	libibIO.so => /media/edz/T7/github/re-Isearch/lib/libibIO.so (0x00007fc4d5a27000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007fc4d5823000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007fc4d5416000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fc4d5078000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007fc4d4e60000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fc4d4a6f000)
	/lib64/ld-linux-x86-64.so.2 (0x00007fc4d6983000)
libibDoc.so:
	linux-vdso.so.1 (0x00007ffdb5e4f000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007f552c8f6000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007f552c6f2000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f552c2e5000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f552bf47000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f552bd2f000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f552b93e000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f552cebd000)
libibIO.so:
	linux-vdso.so.1 (0x00007ffe6b1fc000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f090fba4000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f090f806000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f090f5ee000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f090f1fd000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f09101be000)
libibLocal.so:
	linux-vdso.so.1 (0x00007ffcae43e000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f2ba2c6f000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f2ba3264000)
libibProcess.so:
	linux-vdso.so.1 (0x00007ffc64383000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f3fd7112000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f3fd6d74000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f3fd6b5c000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f3fd676b000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f3fd7723000)
libibSearch.so:
	linux-vdso.so.1 (0x00007ffdb6fe8000)
	libibUtils.so => /media/edz/T7/github/re-Isearch/lib/libibUtils.so (0x00007f5e3bef7000)
	libibLocal.so => /media/edz/T7/github/re-Isearch/lib/libibLocal.so (0x00007f5e3bcf3000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f5e3b8e6000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f5e3b548000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f5e3b330000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f5e3af3f000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f5e3c4fa000)
libibUtils.so:
	linux-vdso.so.1 (0x00007ffdc928d000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f05e0806000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f05e0468000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f05e0250000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f05dfe5f000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f05e0ec6000)
</PRE>

This project was funded through the NGI0 Discovery Fund, a fund established by NLnet with financial support from the European Commission's Next Generation Internet programme, under the aegis of DG Communications Networks, Content and Technology under grant agreement No 825322.


<IMG SRC="https://nlnet.nl/image/logo_nlnet.svg" ALT="NLnet Foundation" height=100> <IMG SRC="https://nlnet.nl/logo/NGI/NGIZero-green.hex.svg" ALT="NGI0 Search" height=100> &nbsp; &nbsp; <IMG SRC="https://ngi.eu/wp-content/uploads/sites/77/2017/10/bandiera_stelle.png" ALT="EU" height=100>

