/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneImageWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneImageWriter.h"

#include "vtkErrorCode.h"
#include "vtkGenericMovieWriter.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkMPEG2Writer.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkTIFFWriter.h"
#include "vtkToolkits.h"

#include <vtkstd/string>
#include <vtksys/SystemTools.hxx>

#ifdef _WIN32
# include "vtkAVIWriter.h"
#else
# ifdef VTK_USE_FFMPEG_ENCODER
#   include "vtkFFMPEGWriter.h"
# endif
#endif

vtkStandardNewMacro(vtkSMAnimationSceneImageWriter);
vtkCxxRevisionMacro(vtkSMAnimationSceneImageWriter, "1.1");
vtkCxxSetObjectMacro(vtkSMAnimationSceneImageWriter,
  ImageWriter, vtkImageWriter);
vtkCxxSetObjectMacro(vtkSMAnimationSceneImageWriter,
  MovieWriter, vtkGenericMovieWriter);
//-----------------------------------------------------------------------------
vtkSMAnimationSceneImageWriter::vtkSMAnimationSceneImageWriter()
{
  this->Magnification = 1;
  this->ErrorCode = 0;
  this->Quality = 1;
  this->ActualSize[0] = this->ActualSize[1] = 0;

  this->MovieWriter = 0;
  this->ImageWriter = 0;
  this->FileCount = 0;

  this->BackgroundColor[0] = this->BackgroundColor[1] = 
    this->BackgroundColor[2] = 0.0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneImageWriter::~vtkSMAnimationSceneImageWriter()
{
  this->SetMovieWriter(0);
  this->SetImageWriter(0);
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveInitialize()
{
  // Create writers.
  if (!this->CreateWriter())
    {
    return false;
    }

  this->UpdateImageSize();

  if (this->MovieWriter)
    {
    this->MovieWriter->SetFileName(this->FileName);
    vtkImageData* emptyImage = this->NewFrame();
    this->MovieWriter->SetInput(emptyImage);
    emptyImage->Delete();

    this->MovieWriter->Start();
    }

  this->FileCount = 0;
  return true;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMAnimationSceneImageWriter::NewFrame()
{
  vtkImageData* image = vtkImageData::New();
  image->SetDimensions(this->ActualSize[0], this->ActualSize[1], 1);
  image->SetScalarTypeToUnsignedChar();
  image->SetNumberOfScalarComponents(3);
  image->AllocateScalars();

  unsigned char rgb[3];
  rgb[0] = 0x0ff & static_cast<int>(this->BackgroundColor[0]*0x0ff);
  rgb[1] = 0x0ff & static_cast<int>(this->BackgroundColor[1]*0x0ff);
  rgb[2] = 0x0ff & static_cast<int>(this->BackgroundColor[2]*0x0ff);
  vtkImageIterator<unsigned char> it(image, image->GetExtent());
  while (!it.IsAtEnd())
    {
    unsigned char* span = it.BeginSpan();
    unsigned char* spanEnd = it.EndSpan();
    while (spanEnd != span)
      {
      *span = rgb[0]; span++;
      *span = rgb[1]; span++;
      *span = rgb[2]; span++;
      }
    it.NextSpan();
    }
  return image;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::Merge(vtkImageData* dest, vtkImageData* src)
{
  vtkImageIterator<unsigned char> inIt(src, src->GetExtent());
  int outextent[6];
  src->GetExtent(outextent);

  // we need to flip Y.
  outextent[2] = dest->GetExtent()[3] - outextent[2];
  outextent[3] = dest->GetExtent()[3] - outextent[3];
  int temp = outextent[2];
  outextent[2] = outextent[3];
  outextent[3] = temp;
  vtkImageIterator<unsigned char> outIt(dest, outextent);

  while (!outIt.IsAtEnd() && !inIt.IsAtEnd())
    {
    unsigned char* spanOut = outIt.BeginSpan();
    unsigned char* spanIn = inIt.BeginSpan();
    unsigned char* outSpanEnd = outIt.EndSpan();
    unsigned char* inSpanEnd = inIt.EndSpan();
    while (outSpanEnd != spanOut && inSpanEnd != spanIn)
      {
      *spanOut = *spanIn;
      ++spanIn;
      ++spanOut;
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveFrame(double vtkNotUsed(time))
{
  vtkImageData* combinedImage = this->NewFrame();

  for (unsigned int cc=0; 
    cc < this->AnimationScene->GetNumberOfViewModules(); cc++)
    {
    vtkSMAbstractViewModuleProxy* view = this->AnimationScene->GetViewModule(cc);
    vtkSMRenderModuleProxy* rmview = vtkSMRenderModuleProxy::SafeDownCast(view);
    if (!rmview)
      {
      continue;
      }
    vtkImageData* capture = rmview->CaptureWindow(this->Magnification);
    this->Merge(combinedImage, capture);
    capture->Delete();
    }

  int errcode = 0;
  if (this->ImageWriter)
    {
    char number[1024];
    sprintf(number, ".%04d", this->FileCount);
    vtkstd::string filename = this->FileName;
    vtkstd::string::size_type dot_pos = filename.rfind(".");
    if(dot_pos != vtkstd::string::npos)
      {
      filename = filename.substr(0, dot_pos);
      }
    filename =  filename + number 
      + vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    this->ImageWriter->SetInput(combinedImage);
    this->ImageWriter->SetFileName(filename.c_str());
    this->ImageWriter->Write();
    errcode = this->ImageWriter->GetErrorCode(); 
    this->FileCount = (!errcode)? this->FileCount + 1 : this->FileCount; 
    }
  else if (this->MovieWriter)
    {
    this->MovieWriter->SetInput(combinedImage);
    this->MovieWriter->Write();

    int alg_error = this->MovieWriter->GetErrorCode();
    int movie_error = this->MovieWriter->GetError();

    if (movie_error && !alg_error)
      {
      //An error that the moviewriter caught, without setting any error code.
      //vtkGenericMovieWriter::GetStringFromErrorCode will result in
      //Unassigned Error. If this happens the Writer should be changed to set
      //a meaningful error code.

      errcode = vtkErrorCode::UserError;      
      }
    else
      {
      //if 0, then everything went well

      //< userError, means a vtkAlgorithm error (see vtkErrorCode.h)
      //= userError, means an unknown Error (Unassigned error)
      //> userError, means a vtkGenericMovieWriter error

      errcode = alg_error;
      }
    }
  combinedImage->Delete();
  if (errcode)
    {
    this->ErrorCode = errcode;
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveFinalize()
{
  if (this->MovieWriter)
    {
    this->MovieWriter->End();
    this->SetMovieWriter(0);
    }
  this->SetImageWriter(0);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::CreateWriter()
{
  this->SetMovieWriter(0);
  this->SetImageWriter(0);

  vtkImageWriter* iwriter =0;
  vtkGenericMovieWriter* mwriter = 0;

  vtkstd::string extension = vtksys::SystemTools::GetFilenameLastExtension(
    this->FileName);
  if (extension == ".jpg" || extension == ".jpeg")
    {
    iwriter = vtkJPEGWriter::New();
    }
  else if (extension == ".tif" || extension == ".tiff")
    {
    iwriter = vtkTIFFWriter::New();
    }
  else if (extension == ".png")
    {
    iwriter = vtkPNGWriter::New();
    }
  else if (extension == ".mpeg" || extension == ".mpg")
    {
    mwriter = vtkMPEG2Writer::New();
    }
#ifdef _WIN32
  else if (extension == ".avi")
    {
    vtkAVIWriter* avi = vtkAVIWriter::New();
    avi->SetQuality(this->Quality);
    avi->SetRate(
      static_cast<int>(this->AnimationScene->GetFrameRate()));
    mwriter = avi;
    }
#else
# ifdef VTK_USE_FFMPEG_ENCODER
  else if (extension == ".avi")
    {
    vtkFFMPEGWriter *aviwriter = vtkFFMPEGWriter::New();
    aviwriter->SetQuality(this->Quality);
    aviwriter->SetRate(
      static_cast<int>(this->AnimationScene->GetFrameRate()));
    mwriter = aviwriter;
    }
# endif
#endif
  else
    {
    vtkErrorMacro("Unknown extension " << extension.c_str());
    return false;
    }

  if (iwriter)
    {
    this->SetImageWriter(iwriter);
    iwriter->Delete();
    }
  if (mwriter)
    {
    this->SetMovieWriter(mwriter);
    mwriter->Delete();
    }
  return true;
}


//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::UpdateImageSize()
{
  int gui_size[2] = {1, 1};
  vtkSMAbstractViewModuleProxy* view = this->AnimationScene->GetViewModule(0);
  if (view)
    {
    view->GetGUISize(gui_size);
    }
  else
    {
    vtkErrorMacro("AnimationScene has no view modules added to it.");
    }
  gui_size[0] *= this->Magnification;
  gui_size[1] *= this->Magnification;
  this->SetActualSize(gui_size);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Quality: " << this->Quality << endl;
  os << indent << "Magnification: " << this->Magnification << endl;
}
