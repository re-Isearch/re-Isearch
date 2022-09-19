/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public final class MergeStatus {
  public final static MergeStatus iNothing = new MergeStatus("iNothing", IBJNI.iNothing_get());
  public final static MergeStatus iOptimize = new MergeStatus("iOptimize");
  public final static MergeStatus iMerge = new MergeStatus("iMerge");
  public final static MergeStatus iCollapse = new MergeStatus("iCollapse");
  public final static MergeStatus iIncremental = new MergeStatus("iIncremental");

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static MergeStatus swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + MergeStatus.class + " with value " + swigValue);
  }

  private MergeStatus(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private MergeStatus(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private MergeStatus(String swigName, MergeStatus swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static MergeStatus[] swigValues = { iNothing, iOptimize, iMerge, iCollapse, iIncremental };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}
