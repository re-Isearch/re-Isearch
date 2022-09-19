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

static const char myDescription[] = "EXIF Plugin";

class EXIF_DOC : public COLONDOC {

public:

EXIF_DOC(PIDBOBJ DbParent, const STRING& Name) : COLONDOC(DbParent, Name)
{
#if USE_LIBMAGIC
  magic_cookie = NULL;
#endif
}

void SourceMIMEContent(const RESULT& Result, STRING *StringPtr) const
{
  if (StringPtr)
  {
    DOCTYPE::Present (Result,  content_type_key, StringPtr);
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




void BeforeIndexing()
{

#if USE_LIBMAGIC
  if (magic_cookie == NULL)
     magic_cookie = magic_open(MAGIC_MIME|MAGIC_SYMLINK|MAGIC_ERROR);
  if (magic_cookie) magic_load(magic_cookie, NULL); // Use default 
#endif
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

 ~EXIF_DOC() { }

private:

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
	// open a stream to read just the necessary parts of the image file
	std::ifstream in_stream(input_file, std::ios::binary);
	if (!in_stream) return -2;

	// parse image EXIF and XMP metadata
	TinyEXIF::EXIFInfo imageEXIF(in_stream);

	// Now open the output stream
	std::ofstream stream(output_file, std::ios::binary);
        if (stream) return -2;

	std::cout << "Key: ";
	// For Key would be nice to use Unique Image ID
	if (!imageEXIF.ImageUniqueID.empty())
		stream <<  imageEXIF.ImageUniqueID;
	else // Need to generate Key
		stream << INODE(input_file).Key();
	stream << std::endl << "Local-Path: "<< input_file << std::endl;


#if USE_LIBMAGIC
	stream << std::endl << content_type_key << ": "<< 
		magic_file (magic_cookie, input_file) << std::endl;
#else
	// We could use the extension but it is unreliable... so we'll just
	// say binary.. and let the application figure out what to do..
	 stream << std::endl << content_type_key << ": " << default_content_type;
#endif

	if (!imageEXIF.Fields) return -3; // No fields

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
		stream << "DateTime: " << imageEXIF.DateTime << std::endl;
	if (!imageEXIF.DateTimeOriginal.empty())
		stream << "DateTimeOriginal: " << imageEXIF.DateTimeOriginal << std::endl;
	if (!imageEXIF.DateTimeDigitized.empty())
		stream << "DateTimeDigitized: " << imageEXIF.DateTimeDigitized << std::endl;
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
	stream << "GeoLocation.GPSDOP " << imageEXIF.GeoLocation.GPSDOP << std::endl;
	stream << "GeoLocation.GPSDifferential: " << imageEXIF.GeoLocation.GPSDifferential << std::endl;
	if (!imageEXIF.GeoLocation.GPSMapDatum.empty())
		stream << "GeoLocation.GPSMapDatum: " << imageEXIF.GeoLocation.GPSMapDatum << std::endl;
	if (!imageEXIF.GeoLocation.GPSTimeStamp.empty())
		stream << "GeoLocation.GPSTimeStamp: " << imageEXIF.GeoLocation.GPSTimeStamp << std::endl;
	if (!imageEXIF.GeoLocation.GPSDateStamp.empty())
		stream << "GeoLocation.GPSDateStamp: " << imageEXIF.GeoLocation.GPSDateStamp << std::endl;
	if (imageEXIF.GPano.hasPosePitchDegrees())
		stream << "GPano.PosePitchDegrees: " << imageEXIF.GPano.PosePitchDegrees << std::endl;
	if (imageEXIF.GPano.hasPoseRollDegrees())
		stream << "GPano.PoseRollDegrees: " << imageEXIF.GPano.PoseRollDegrees << std::endl;
	return 0;
}



} /* end Class definition */ ;


// Stubs for dynamic loading
extern "C" {
  EXIF_DOC *  __plugin_exif_create (IDBOBJ * parent, const STRING& Name)
  {
    return new EXIF_DOC (parent, Name);
  }
  int          __plugin_exif_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_exif_query (void) { return myDescription; }
}


