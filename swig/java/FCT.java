/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class FCT {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected FCT(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(FCT obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      IBJNI.delete_FCT(swigCPtr);
    }
    swigCPtr = 0;
  }

  public FCT() {
    this(IBJNI.new_FCT(), true);
  }

  public void AddEntry(FC FcRecord) {
    IBJNI.FCT_AddEntry(swigCPtr, this, FC.getCPtr(FcRecord), FcRecord);
  }

  public FC GetEntry(long Index) {
    return new FC(IBJNI.FCT_GetEntry(swigCPtr, this, Index), true);
  }

  public boolean IsSorted() {
    return IBJNI.FCT_IsSorted(swigCPtr, this);
  }

  public boolean IsEmpty() {
    return IBJNI.FCT_IsEmpty(swigCPtr, this);
  }

  public long GetTotalEntries() {
    return IBJNI.FCT_GetTotalEntries(swigCPtr, this);
  }

  public void Reverse() {
    IBJNI.FCT_Reverse(swigCPtr, this);
  }

  public void SortByFc() {
    IBJNI.FCT_SortByFc(swigCPtr, this);
  }

  public int Refcount_() {
    return IBJNI.FCT_Refcount_(swigCPtr, this);
  }

}
