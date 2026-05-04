#ifdef _MSC_VER
#include <windows.h>
#endif
#include "TinyEXIF.h"
#include <iostream> // stream
#include <fstream>  // std::ifstream
#include <vector>   // std::vector
#include <iomanip>  // std::setprecision

#include "common.hxx"
#include "colondoc.hxx"
#include "process.hxx"
#include "doc_conf.hxx"


#include "inode.hxx"

#if USE_LIBMAGIC
# undef _MAGIC_H
# include <magic.h>
#endif



static const char *content_type_key = "Content-Type";
static const char *default_content_type = "binary/octet-stream";

static const char myDescription[] = "EXIF Plugin. Reads the EXIF Info in JPEGs";

class EXIF_DOC : public COLONDOC {

public:

  EXIF_DOC(PIDBOBJ DbParent, const STRING& Name) : COLONDOC(DbParent, Name)
  {
#if USE_LIBMAGIC
    magic_cookie = NULL;
#endif
    DateField = "DateTime";
    DateModifiedField = "DateTimeDigitized";
    DateCreatedField = "DateTimeOriginal";
  }


  void LoadFieldTable()
  {
    if (Db)
    {
      Db->AddFieldType(DateField, FIELDTYPE::date);
      Db->AddFieldType(DateCreatedField, FIELDTYPE::date);
      Db->AddFieldType(DateModifiedField, FIELDTYPE::date);
    }
    COLONDOC::LoadFieldTable();
  }


  void SourceMIMEContent(const RESULT& Result, STRING *StringPtr) const
  {
    if (StringPtr)
    {
      DOCTYPE::Present (Result,  content_type_key, StringPtr);
      StringPtr->Trim(STRING::both);
      if (StringPtr->GetLength() == 0) SourceMIMEContent(StringPtr);
    }
  }

  void SourceMIMEContent(STRING *stringPtr) const
  {
    *stringPtr = default_content_type; 
  }


  const char *Description(PSTRLIST List) const
  {
    List->AddEntry (Doctype);
    COLONDOC::Description(List);
    return myDescription;
  }



   void ParseRecords (const RECORD& FileRecord);

void BeforeIndexing()
{
#if USE_LIBMAGIC
  if (magic_cookie == NULL)
     magic_cookie = magic_open(MAGIC_MIME|MAGIC_SYMLINK|MAGIC_ERROR);
  if (magic_cookie) magic_load(magic_cookie, NULL); // Use default 
#endif
}

bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  if (ResultRecord.GetRecordStart() == 0)
    {
      STRING path;
      Db->GetFieldData (ResultRecord, "Local-Path", &path, this);
      path = path.Strip( STRING::both );

      if (path.IsEmpty() || GetFileSize(path) < 10)
        {
          return false;
        }
      if (StringBuffer) *StringBuffer = path;
      return true;
    }
  return false;
}



void AfterIndexing()
{
#if USE_LIBMAGIC
  if (magic_cookie != NULL)
    {
      magic_close (magic_cookie);
      magic_cookie = NULL;
    }
#endif
}


void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
       STRING source;
      // Read here the source file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      if (GetResourcePath(ResultRecord, &source) && GetFileSize(source) > 10)
        StringBuffer->CatFile( source );
    }
  else
    COLONDOC::DocPresent(ResultRecord,ElementSet,RecordSyntax,StringBuffer);
}


void Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(BRIEF_MAGIC))
    COLONDOC::Present(ResultRecord, "Description" ,RecordSyntax,StringBuffer);
  if (StringBuffer->IsEmpty())
    COLONDOC::Present(ResultRecord,ElementSet,RecordSyntax,StringBuffer);

}

 ~EXIF_DOC() {
#if USE_LIBMAGIC
  AfterIndexing();
#endif
 }

private:


// Convert the EXIF date format to ISO 8601
STRING date_convert(const char *exif_date) const
{
  int yyyy, mm, dd;
  int _hh =0, _mm = 0, _ss = 0;
  int n;

  // YYYY:MM:DD HH:MM:SS" with time shown in 24-hour format, and the date and time separated by one blank character (hex 20). 
  if ( (n = sscanf (exif_date,"%d:%d:%d %d:%d:%d", &yyyy, &mm, &dd, &_hh, &_mm, &_ss)) >= 3) {
     STRING s;
     if (n == 3)
	s.form("%04d%02d%02d", yyyy, mm, dd);
     else
	s.form("%04d%02d%02dT%02d:%02d:%02d", yyyy, mm, dd, _hh, _mm, _ss);
     return s;
  }
  return exif_date;
}


#if USE_LIBMAGIC
  magic_t magic_cookie;
#endif


// Uses TinyEXIF, a tiny, lightweight C++ library for parsing the metadata existing inside JPEG files.
//
// Even if no metadata is available we shall have at least two fields:
//	Key
//	LocalPath
//
int gen_metadata(const char *input_file, const char *output_file) 
{
#if USE_LIBMAGIC
        STRING content_type = magic_file (magic_cookie, input_file);
	if (!content_type.IsEmpty() && content_type.SearchAny("jpeg") == 0) // Not a JPEG
	   return -4;
#endif
	// open a stream to read just the necessary parts of the image file
	std::ifstream in_stream(input_file, std::ios::binary);
	if (!in_stream) return -2;

	// parse image EXIF and XMP metadata
	TinyEXIF::EXIFInfo imageEXIF(in_stream);

	// Now open the output stream
	std::ofstream stream(output_file, std::ios::binary);
        if (!stream) return -2;

	stream << "Key: ";
	// For Key would be nice to use Unique Image ID
	if (!imageEXIF.ImageUniqueID.empty())
		stream <<  imageEXIF.ImageUniqueID;
	else // Need to generate Key
		stream << INODE(input_file).Key();
	stream << std::endl << "Local-Path: "<< input_file << std::endl;


#if USE_LIBMAGIC
	if (!content_type.IsEmpty())
	   stream << content_type_key << ": "<< content_type << std::endl;
	else
#endif
	// We could use the extension but it is unreliable... so we'll just
	// say binary.. and let the application figure out what to do..
	 stream << content_type_key << ": " << default_content_type << std::endl;

	if (!imageEXIF.Fields) {
	  stream.close();
	  return -3; // No fields
	}

	// print extracted metadata
	if (imageEXIF.ImageWidth || imageEXIF.ImageHeight)
		stream << "ImageResolution: " << imageEXIF.ImageWidth << "x" << imageEXIF.ImageHeight << " pixels" << std::endl;
	if (imageEXIF.RelatedImageWidth || imageEXIF.RelatedImageHeight)
		stream << "RelatedImageResolution: " << imageEXIF.RelatedImageWidth << "x" << imageEXIF.RelatedImageHeight << " pixels" << std::endl;
	if (!imageEXIF.ImageDescription.empty())
		stream << "Description: " << imageEXIF.ImageDescription << std::endl;
	if (!imageEXIF.Make.empty() || !imageEXIF.Model.empty())
		stream << "CameraModel: " << imageEXIF.Make << " - " << imageEXIF.Model << std::endl;
	if (!imageEXIF.SerialNumber.empty())
		stream << "SerialNumber: " << imageEXIF.SerialNumber << std::endl;
	if (imageEXIF.Orientation)
		stream << "Orientation: " << imageEXIF.Orientation << std::endl;
	if (imageEXIF.XResolution || imageEXIF.YResolution || imageEXIF.ResolutionUnit)
		stream << "Resolution: " << imageEXIF.XResolution << "x" << imageEXIF.YResolution << " (" << imageEXIF.ResolutionUnit << ")\n";
	if (imageEXIF.BitsPerSample)
		stream << "BitsPerSample: " << imageEXIF.BitsPerSample << std::endl;
	if (!imageEXIF.Software.empty())
		stream << "Software: " << imageEXIF.Software << std::endl;
	if (!imageEXIF.DateTime.empty())
		stream << "DateTime: " <<  date_convert(imageEXIF.DateTime.c_str()) << std::endl;
	if (!imageEXIF.DateTimeOriginal.empty())
		stream << "DateTimeOriginal: " << date_convert(imageEXIF.DateTimeOriginal.c_str()) << std::endl;
	if (!imageEXIF.DateTimeDigitized.empty())
		stream << "DateTimeDigitized: " << date_convert(imageEXIF.DateTimeDigitized.c_str()) << std::endl;
	if (!imageEXIF.SubSecTimeOriginal.empty())
		stream << "SubSecTimeOriginal: " << imageEXIF.SubSecTimeOriginal << std::endl;
	if (!imageEXIF.Copyright.empty())
		stream << "Copyright: " << imageEXIF.Copyright << std::endl;
	stream << "ExposureTime: " << std::setprecision(10) << imageEXIF.ExposureTime << " s" << std::endl;
	stream << "FNumber: " << imageEXIF.FNumber << std::endl;
	stream << "ExposureProgram: " << imageEXIF.ExposureProgram << std::endl;
	stream << "ISOSpeed: " << imageEXIF.ISOSpeedRatings << std::endl;
	stream << "ShutterSpeedValue: " << std::setprecision(10) << imageEXIF.ShutterSpeedValue << std::endl;
	stream << "ApertureValue: " << std::setprecision(10) << imageEXIF.ApertureValue << std::endl;
	stream << "BrightnessValue: " << std::setprecision(10) << imageEXIF.BrightnessValue << std::endl;
	stream << "ExposureBiasValue: " << imageEXIF.ExposureBiasValue << std::endl;
	stream << "SubjectDistance: " << imageEXIF.SubjectDistance << std::endl;
	stream << "FocalLength: " << imageEXIF.FocalLength << " mm" << std::endl;
	stream << "Flash: " << imageEXIF.Flash << std::endl;
	if (!imageEXIF.SubjectArea.empty()) {
		stream << "SubjectArea: ";
		for (uint16_t val: imageEXIF.SubjectArea)
			stream << " " << val;
		stream << std::endl;
	}
	stream << "MeteringMode: " << imageEXIF.MeteringMode << std::endl;
	stream << "LightSource: " << imageEXIF.LightSource << std::endl;
	stream << "ProjectionType: " << imageEXIF.ProjectionType << std::endl;
	if (imageEXIF.Calibration.FocalLength != 0)
		stream << "Calibration.FocalLength: " << imageEXIF.Calibration.FocalLength << " pixels" << std::endl;
	if (imageEXIF.Calibration.OpticalCenterX != 0)
		stream << "Calibration.OpticalCenterX: " << imageEXIF.Calibration.OpticalCenterX << " pixels" << std::endl;
	if (imageEXIF.Calibration.OpticalCenterY != 0)
		stream << "Calibration.OpticalCenterY: " << imageEXIF.Calibration.OpticalCenterY << " pixels" << std::endl;
	stream << "LensInfo.FStopMin: " << imageEXIF.LensInfo.FStopMin << std::endl;
	stream << "LensInfo.FStopMax: " << imageEXIF.LensInfo.FStopMax << std::endl;
	stream << "LensInfo.FocalLengthMin: " << imageEXIF.LensInfo.FocalLengthMin << " mm" << std::endl;
	stream << "LensInfo.FocalLengthMax: " << imageEXIF.LensInfo.FocalLengthMax << " mm" << std::endl;
	stream << "LensInfo.DigitalZoomRatio: " << imageEXIF.LensInfo.DigitalZoomRatio << std::endl;
	stream << "LensInfo.FocalLengthIn35mm: " << imageEXIF.LensInfo.FocalLengthIn35mm << std::endl;
	stream << "LensInfo.FocalPlaneXResolution: " << std::setprecision(10) << imageEXIF.LensInfo.FocalPlaneXResolution << std::endl;
	stream << "LensInfo.FocalPlaneYResolution: " << std::setprecision(10) << imageEXIF.LensInfo.FocalPlaneYResolution << std::endl;
	stream << "LensInfo.FocalPlaneResolutionUnit: " << imageEXIF.LensInfo.FocalPlaneResolutionUnit << std::endl;
	if (!imageEXIF.LensInfo.Make.empty() || !imageEXIF.LensInfo.Model.empty())
		stream << "LensInfo.Model: " << imageEXIF.LensInfo.Make << " - " << imageEXIF.LensInfo.Model << std::endl;
	if (imageEXIF.GeoLocation.hasLatLon()) {
		stream << "GeoLocation.Latitude: " << std::setprecision(10) << imageEXIF.GeoLocation.Latitude << std::endl;
		stream << "GeoLocation.Longitude: " << std::setprecision(10) << imageEXIF.GeoLocation.Longitude << std::endl;
	}
	if (imageEXIF.GeoLocation.hasAltitude()) {
		stream << "GeoLocation.Altitude: " << imageEXIF.GeoLocation.Altitude << " m" << std::endl;
		stream << "GeoLocation.AltitudeRef: " << (int)imageEXIF.GeoLocation.AltitudeRef << std::endl;
	}
	if (imageEXIF.GeoLocation.hasRelativeAltitude())
		stream << "GeoLocation.RelativeAltitude: " << imageEXIF.GeoLocation.RelativeAltitude << " m" << std::endl;
	if (imageEXIF.GeoLocation.hasOrientation()) {
		stream << "GeoLocation.RollDegree: " << imageEXIF.GeoLocation.RollDegree << std::endl;
		stream << "GeoLocation.PitchDegree: " << imageEXIF.GeoLocation.PitchDegree << std::endl;
		stream << "GeoLocation.YawDegree: " << imageEXIF.GeoLocation.YawDegree << std::endl;
	}
	if (imageEXIF.GeoLocation.hasSpeed()) {
		stream << "GeoLocation.SpeedX: " << imageEXIF.GeoLocation.SpeedX << " m/s" << std::endl;
		stream << "GeoLocation.SpeedY: " << imageEXIF.GeoLocation.SpeedY << " m/s" << std::endl;
		stream << "GeoLocation.SpeedZ: " << imageEXIF.GeoLocation.SpeedZ << " m/s" << std::endl;
	}
	if (imageEXIF.GeoLocation.AccuracyXY > 0 || imageEXIF.GeoLocation.AccuracyZ > 0)
		stream << "GeoLocation.GPSAccuracy: XY " << imageEXIF.GeoLocation.AccuracyXY << " m" << " Z " << imageEXIF.GeoLocation.AccuracyZ << " m" << std::endl;
	stream << "GeoLocation.GPSDOP: " << imageEXIF.GeoLocation.GPSDOP << std::endl;
	stream << "GeoLocation.GPSDifferential: " << imageEXIF.GeoLocation.GPSDifferential << std::endl;
	if (!imageEXIF.GeoLocation.GPSMapDatum.empty())
		stream << "GeoLocation.GPSMapDatum: " << imageEXIF.GeoLocation.GPSMapDatum << std::endl;
	if (!imageEXIF.GeoLocation.GPSTimeStamp.empty())
		stream << "GeoLocation.GPSTimeStamp: " << imageEXIF.GeoLocation.GPSTimeStamp << std::endl;
	if (!imageEXIF.GeoLocation.GPSDateStamp.empty())
		stream << "GeoLocation.GPSDateStamp: " << date_convert(imageEXIF.GeoLocation.GPSDateStamp.c_str()) << std::endl;
	if (imageEXIF.GPano.hasPosePitchDegrees())
		stream << "GPano.PosePitchDegrees: " << imageEXIF.GPano.PosePitchDegrees << std::endl;
	if (imageEXIF.GPano.hasPoseRollDegrees())
		stream << "GPano.PoseRollDegrees: " << imageEXIF.GPano.PoseRollDegrees << std::endl;
	return 0;
}

} /* end Class definition */ ;


void EXIF_DOC::ParseRecords (const RECORD& FileRecord)
{
   STRING fn (FileRecord.GetFullFileName());
   STRING s;

    Db->ComposeDbFn (&s, DbExtCat);
    if (MkDir(s, 0, true) == -1) // Force creation
      {
        message_log (LOG_ERRNO, "Can't create meta directory '%s'", s.c_str() );
        return;
       }

    const STRING key = INODE(fn).Key();

    STRING outfile =  AddTrailingSlash(s);
    outfile.Cat (((long)key.CRC16()) % 1000);
    if (MkDir(outfile, 0, true) == -1)
      outfile = s; // Can't make it
    AddTrailingSlash(&outfile);
    outfile.Cat (key);


#if USE_LIBMAGIC
    if (magic_cookie == NULL) BeforeIndexing() ;
#endif
    int result = gen_metadata(fn, outfile);

    if (result == 0 || result == -3) 
    {
       RECORD r (FileRecord);
       r.SetRecordStart(0);
       r.SetRecordEnd(0);
       r.SetFullFileName ( outfile );
       r.SetKey( key );
       COLONDOC::ParseRecords (r);
    } else if (result == -4) {
cerr << "NOTE A JPEG!!!!" << endl;
       // Not a JPEG.. let AUTODETECT figure out what to do....
       RECORD r (FileRecord);
       r.SetDocumentType ( "AUTODETECT" );
       // Db->DocTypeAddRecord(r);
       DOCTYPE::ParseRecords( r);
    } else message_log(LOG_INFO, "EXIF error. Skipping %s", fn.c_str());
}



// Stubs for dynamic loading
extern "C" {
  EXIF_DOC *  __plugin_exif_create (IDBOBJ * parent, const STRING& Name)
  {
    return new EXIF_DOC (parent, Name);
  }
  int          __plugin_exif_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_exif_query (void) { return myDescription; }
}


