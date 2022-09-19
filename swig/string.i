%typedef const String& constStringRef
%typedef String& StringRef
%typedef const String constString

%typemap(perl5,in) constStringRef  (String tmp), StringRef (String tmp) {
  char* str = SvPV($source, PL_na);
  tmp.set(str, strlen(str));
  $target = &tmp;
}

//
// A temporary is allocated and needs to be destroyed.
// Convert to Perl string, we don't want to get an
// interface to the String class.
//
%typemap(perl5,out) constString, String {
  ST(argvi) = sv_newmortal();
  sv_setpv(ST(argvi++), (char*)(*_result));
  delete _result;
}

//
// Annoying: match when returning a ref or a pointer.
// If it's a ref, the _result is really a pointer to
// a local variable that cannot be free'd. If it's a 
// pointer, the _result contains the pointer and we
// probably want to free it (check other typemap facilities
// to solve this ?)
//
%typemap(perl5,out) String * {
  ST(argvi) = sv_newmortal();
  sv_setpvn(ST(argvi++), (char*)(*_result), _result->length());
}
