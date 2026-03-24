/*
Go from SearchResult to IRSET .....
Needs IDB Parent
*/

#if 0

#include "EmbeddingIndexer.hpp"

PIRSET  EmbeddingIndexer::search(const STRING &fieldname, const STRING &query, IDB *Parent) {
  float           boost = 1.0;
  PIRSET          pirset = new IRSET ( Parent,  TotalHits + 1);
  IRESULT         iresult;
  MDTREC          mdtrec;
  FC              fc;

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);

  // Here gplist from SearchResult sentence_id and a loop 
  auto results = manager.search(fieldname.toStdString(), query.toStdString() );
  for (const auto &r : results) {
   const GPTYPE gp = r.sentence_id;
   float mult = boost; //  + (r.token_end - r.token_start)/50.0; 
   size_t w = Parent->GetMainMdt ()->LookupByGp (gp);
   if (w == 0) continue; // Could not find GP
   if (Parent->GetMainMdt ()->GetEntry (w, &mdtrec)) {
      if (mdtrec.GetDeleted()) continue; // Deleted entry
      iresult.SetMdtIndex (w);
      iresult.SetScore(r.score * mult);
      fc.SetFieldStart(gp);
      fc.SetFieldEnd(gp + r.span); 
      iresult.SetHitTable (fc);
      // Add entry
      pirset->AddEntry (iresult, true);
   }
  }

   return pirset;
*/

#endif
