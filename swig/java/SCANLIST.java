/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class SCANLIST {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected SCANLIST(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(SCANLIST obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      IBJNI.delete_SCANLIST(swigCPtr);
    }
    swigCPtr = 0;
  }

  public SCANLIST(SCANLIST Table) {
    this(IBJNI.new_SCANLIST(SCANLIST.getCPtr(Table), Table), true);
  }

  public void Reverse() {
    IBJNI.SCANLIST_Reverse(swigCPtr, this);
  }

  public boolean IsEmpty() {
    return IBJNI.SCANLIST_IsEmpty(swigCPtr, this);
  }

  public long GetTotalEntries() {
    return IBJNI.SCANLIST_GetTotalEntries(swigCPtr, this);
  }

  public SCANOBJ GetEntry(long Index) {
    return new SCANOBJ(IBJNI.SCANLIST_GetEntry(swigCPtr, this, Index), true);
  }

}