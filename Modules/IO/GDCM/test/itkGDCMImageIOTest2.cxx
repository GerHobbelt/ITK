/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "itkGDCMImageIO.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"
#include "itkTestingMacros.h"

#include <sstream>

// Specific ImageIO test

// Take as input any kind of image (typically RAW) and will create a valid
// DICOM MR image out of the raw image.
int
itkGDCMImageIOTest2(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " InputFile OutputDicomRoot" << std::endl;
    return EXIT_FAILURE;
  }
  const char * input = argv[1];
  const char * output = argv[2];
  using ImageType = itk::Image<unsigned char, 3>;
  using ReaderType = itk::ImageFileReader<ImageType>;
  using WriterType = itk::ImageFileWriter<ImageType>;

  auto reader = ReaderType::New();
  reader->SetFileName(input);
  try
  {
    reader->Update();
  }
  catch (const itk::ExceptionObject & e)
  {
    std::cerr << "exception in file writer " << std::endl;
    std::cerr << e << std::endl;
    return EXIT_FAILURE;
  }

  itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();
  // reader->GetOutput()->Print(std::cout);

  itk::MetaDataDictionary & dict = dicomIO->GetMetaDataDictionary();
  std::string               tagkey, value, commatagkey, commavalue;
  tagkey = "0002|0002";
  value = "1.2.840.10008.5.1.4.1.1.4"; // Media Storage SOP Class UID
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0008|0060"; // Modality
  value = "MR";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0008|0008"; // Image Type
  value = "DERIVED\\SECONDARY";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0008|0064"; // Conversion Type
  value = "DV";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|5100"; // Patient Position
  value = "HFS";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0020|1040"; // Position Reference Indicator
  value = "";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  commatagkey = "0018,0020"; // Scanning Sequence
  commavalue = "GR";
  itk::EncapsulateMetaData<std::string>(dict, commatagkey, commavalue);
  tagkey = "0018|0021"; // Sequence Variant
  value = "SS\\SP";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|0022"; // Scan Options
  value = "";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|0023"; // MRA Acquisition Type
  value = "";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|0080"; // Repetition Time
  value = "";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|0081"; // Echo Time
  value = "";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|0091"; // Echo Train Length
  value = "";
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);
  tagkey = "0018|0050"; // Slice Thickness
  std::ostringstream os;
  os << reader->GetImageIO()->GetSpacing(2);
  value = os.str();
  itk::EncapsulateMetaData<std::string>(dict, tagkey, value);

  // Construct different filename based on the root given
  std::string output_j2k = output;
  output_j2k += "-j2k.dcm";
  std::string output_jpll = output;
  output_jpll += "-jpll.dcm";
  std::string output_raw = output;
  output_raw += "-raw.dcm";

  auto writer = WriterType::New();
  writer->SetImageIO(dicomIO);
  writer->SetInput(reader->GetOutput());
  writer->UseInputMetaDataDictionaryOff();

  // Save as JPEG 2000 Lossless
  // Explicitly specify which compression type to use
  dicomIO->SetCompressionType(itk::GDCMImageIO::CompressionEnum::JPEG2000);
  // Request compression of the ImageIO
  writer->UseCompressionOn();
  writer->SetFileName(output_j2k.c_str());
  try
  {
    writer->Update();
  }
  catch (const itk::ExceptionObject & e)
  {
    std::cerr << "exception in file writer " << std::endl;
    std::cerr << e << std::endl;
    return EXIT_FAILURE;
  }

  // Save as JPEG Lossless
  dicomIO->SetCompressionType(itk::GDCMImageIO::CompressionEnum::JPEG);
  writer->SetFileName(output_jpll.c_str());
  try
  {
    writer->Update();
  }
  catch (const itk::ExceptionObject & e)
  {
    std::cerr << "exception in file writer " << std::endl;
    std::cerr << e << std::endl;
    return EXIT_FAILURE;
  }

  // Save as raw
  writer->UseCompressionOff();
  writer->SetFileName(output_raw.c_str());

  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());

  // Try to read tags from the written file
  auto outputReader = ReaderType::New();
  outputReader->SetFileName(writer->GetFileName());
  try
  {
    outputReader->Update();
  }
  catch (const itk::ExceptionObject & error)
  {
    std::cerr << "Error: exception in file reader " << std::endl;
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
  auto & inputDict = outputReader->GetOutput()->GetMetaDataDictionary();
  // DICOM tags can be specified using pipe or comma separators for
  // writing in the metadata dictionary, but are always read back with
  // the pipe separator.
  commatagkey[4] = '|';
  auto tagIt = inputDict.Find(commatagkey);
  if (tagIt == inputDict.End())
  {
    std::cerr << "Error: Tag " << commatagkey << " expected to be in file but missing" << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    itk::MetaDataObject<std::string>::ConstPointer tagvalue =
      dynamic_cast<const itk::MetaDataObject<std::string> *>(tagIt->second.GetPointer());
    if (tagvalue)
    {
      if (tagvalue->GetMetaDataObjectValue() != commavalue)
      {
        std::cerr << "Error: Written tag value was (" << commavalue << "), read value was ("
                  << tagvalue->GetMetaDataObjectValue() << ")" << std::endl;
        return EXIT_FAILURE;
      }
    }
    else
    {
      std::cerr << "Error: Tag value for tag (" << commatagkey << ") is missing" << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
