#ifndef _PLUGIN_HXX
#define _PLUGIN_HXX

// Stubs for dynamic loading
#define define_C_plugin(name, classname, parentdoc, description) \
  static const char *my_description = description; \
  extern "C" { \
  TEXTDOC * __plugin_##name##_create (IDBOBJ *parent, const STRING& Name) { \
    return new classname (parent, Name); } \
  int          __plugin_##name##_id  (void) { return DoctypeDefVersion; } \
  const char  *__plugin__#name##_query (void) { return my_description }} \
const char *classname::Description(PSTRLIST List) const { \
  if (List) { List->AddEntry (Doctype); parentdoc::Description(List); } \
  return my_description; }

#endif
