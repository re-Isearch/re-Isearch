/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
typedef enum {
  DefaultRecordSyntax, // Unknown
  UnimarcRecordSyntax,
  IntermarcRecordSyntax,
  CCFRecordSyntax,
  USmarcRecordSyntax,
  UKmarcRecordSyntax,
  NormarcRecordSyntax,
  LibrismarcRecordSyntax,
  DanmarcRecordSyntax,
  FinmarcRecordSyntax,
  MABRecordSyntax,
  CanmarcRecordSyntax,
  SBNRecordSyntax,
  PicamarcRecordSyntax,
  AusmarcRecordSyntax,
  IbermarcRecordSyntax,
  CatmarcRecordSyntax,
  MalmarcRecordSyntax,
  ExplainRecordSyntax,
  SUTRSRecordSyntax,
  OPACRecordSyntax,
  SummaryRecordSyntax,
  GRS0RecordSyntax,
  GRS1RecordSyntax,
  ESTaskPackageRecordSyntax,
  fragmentRecordSyntax,
  PDFRecordSyntax,
  postscriptRecordSyntax,
  HtmlRecordSyntax,
  TiffRecordSyntax,
  jpegTDRecordSyntax,
  pngRecordSyntax,
  mpegRecordSyntax,
  sgmlRecordSyntax,
  XmlRecordSyntax,
  applicationXMLRecordSyntax,
  tiffBRecordSyntax,
  wavRecordSyntax,
  SQLRSRecordSyntax,
  Latin1RecordSyntax,
  PostscriptRecordSyntax,
  CXFRecordSyntax,
  SgmlRecordSyntax,
  ADFRecordSyntax,
  // Private
  DVBHtmlRecordSyntax, // DVB Private
  TextHighlightRecordSyntax,
  HTMLHighlightRecordSyntax,
  XMLHighlightRecordSyntax
} RecordSyntax_t;

// Convert String xxx.xxxx.xxx -> Internal OID
RecordSyntax_t RecordSyntaxID(const STRING& String);
// Convert Internal OID to xxx.xxx.xxx string
const char    *ID2RecordSyntax(RecordSyntax_t);
