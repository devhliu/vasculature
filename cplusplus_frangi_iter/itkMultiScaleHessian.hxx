#ifndef __itkMultiScaleHessian_hxx
#define __itkMultiScaleHessian_hxx

#include "itkMultiScaleHessian.h"

#include "itkImageRegionIterator.h"
#include "itkCastImageFilter.h"
#include "itkThresholdImageFilter.h"
#include "itkInverseDeconvolutionImageFilter.h"
#include "itkProjectedLandweberDeconvolutionImageFilter.h"
#include "itkParametricBlindLeastSquaresDeconvolutionImageFilter.h"
#include "itkRichardsonLucyDeconvolutionImageFilter.h"
#include "itkIterativeDeconvolutionImageFilter.h"
#include "itkLaplacianSharpeningImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkSqrtImageFilter.h"
#include "vnl/vnl_math.h"


template <typename TInputImage, typename THessianImage, typename TOutputImage>
MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::MultiScaleHessian()
    : MultiScaleHessian<TInputImage, THessianImage,
                        TOutputImage>::MultiScaleHessian(true, 0.3, 6.0, 10,
                                                         false, false)
{
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::MultiScaleHessian(
    const bool nonNeg, double const& sigmaMin, double const& sigmaMax,
    const unsigned int nbSigma, const bool generateScale,
    const bool generateHessian)
    : m_NonNegativeHessianBasedMeasure{nonNeg}, m_SigmaMinimum{sigmaMin},
      m_SigmaMaximum{sigmaMax}, m_NumberOfSigmaSteps{nbSigma},
      m_GenerateScalesOutput{generateScale},
      m_GenerateHessianOutput{generateHessian}
{

  m_SigmaStepMethod = Self::LogarithmicSigmaSteps;

  m_HessianFilter = HessianFilterType::New();
  m_HessianToMeasureFilter = HessianToMeasureFilterType::New();
  m_UpdateBuffer = UpdateBufferType::New();

  typename ScalesImageType::Pointer scalesImage = ScalesImageType::New();
  typename HessianImageType::Pointer hessianImage = HessianImageType::New();
  this->itk::ProcessObject::SetNumberOfRequiredOutputs(3);
  this->itk::ProcessObject::SetNthOutput(1, scalesImage.GetPointer());
  this->itk::ProcessObject::SetNthOutput(2, hessianImage.GetPointer());
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::
    EnlargeOutputRequestedRegion(itk::DataObject* output)
{
  output->SetRequestedRegionToLargestPossibleRegion();
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
typename MultiScaleHessian<TInputImage, THessianImage,
                           TOutputImage>::DataObjectPointer
MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::MakeOutput(
    DataObjectPointerArraySizeType idx)
{
  switch (idx)
  {
  case 1:
    return ScalesImageType::New().GetPointer();
  case 2:
    return HessianImageType::New().GetPointer();
  default:
    return Superclass::MakeOutput(idx);
  }
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::AllocateUpdateBuffer()
{
  typename TOutputImage::Pointer output = this->GetOutput();

  // This copies meta data describing the output such as origin, spacing and the
  // largest region.
  m_UpdateBuffer->CopyInformation(output);
  m_UpdateBuffer->SetRequestedRegion(output->GetRequestedRegion());
  m_UpdateBuffer->SetBufferedRegion(output->GetBufferedRegion());
  m_UpdateBuffer->Allocate();

  // Update buffer is used for > comparisons.
  if (m_NonNegativeHessianBasedMeasure)
  {
    m_UpdateBuffer->FillBuffer(itk::NumericTraits<BufferValueType>::Zero);
  }
  else
  {
    m_UpdateBuffer->FillBuffer(
        itk::NumericTraits<BufferValueType>::NonpositiveMin());
  }
}

template <class TInputImage, class THessianImage, class TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::SetFrangiOnly(
    bool value)
{
  m_HessianToMeasureFilter->SetFrangiOnly(value);
}

template <class TInputImage, class THessianImage, class TOutputImage>
bool MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::GetFrangiOnly()
{
  return m_HessianToMeasureFilter->GetFrangiOnly();
}

template <class TInputImage, class THessianImage, class TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::SetBrightBlood(bool value)
{
  m_HessianToMeasureFilter->SetBrightObject(value);
}

template <class TInputImage, class THessianImage, class TOutputImage>
bool MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::GetBrightBlood()
{
  return m_HessianToMeasureFilter->GetBrightObject();
}

template <class TInputImage, class THessianImage, class TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::SetScaleObject(bool value)
{
  m_HessianToMeasureFilter->SetScaleObjectnessMeasure(value);
}

template <class TInputImage, class THessianImage, class TOutputImage>
bool MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::GetScaleObject()
{
  return m_HessianToMeasureFilter->GetScaleObjectnessMeasure();
}

template <class TInputImage, class THessianImage, class TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::SetAlpha(
    double value)
{
  m_HessianToMeasureFilter->SetAlpha(value);
}

template <class TInputImage, class THessianImage, class TOutputImage>
double MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::GetAlpha()
{
  return m_HessianToMeasureFilter->GetAlpha();
}

template <class TInputImage, class THessianImage, class TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::SetBeta(
    double value)
{
  m_HessianToMeasureFilter->SetBeta(value);
}

template <class TInputImage, class THessianImage, class TOutputImage>
double MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::GetBeta()
{
  return m_HessianToMeasureFilter->GetBeta();
}

template <class TInputImage, class THessianImage, class TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::SetC(
    double value)
{
  m_HessianToMeasureFilter->SetC(value);
}

template <class TInputImage, class THessianImage, class TOutputImage>
double MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::GetC()
{
  return m_HessianToMeasureFilter->GetC();
}

// =============================================================================
// Called by the getoutput into
// AnisotropicDiffusionVesselEnhancementImageFilter. This generates a
// multi-scale Hessian to compute vesselness measure at different sigma.
// =============================================================================
template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::GenerateData()
{
  this->GetOutput()->SetBufferedRegion(this->GetOutput()->GetRequestedRegion());
  this->GetOutput()->Allocate();

  if (m_HessianToMeasureFilter.IsNull())
  {
    itkExceptionMacro(" HessianToMeasure filter is not set. Use "
                      "SetHessianToMeasureFilter() ");
  }

  if (m_GenerateScalesOutput)
  {
    typename ScalesImageType::Pointer scalesImage =
        dynamic_cast<ScalesImageType*>(this->itk::ProcessObject::GetOutput(1));

    scalesImage->SetBufferedRegion(scalesImage->GetRequestedRegion());
    scalesImage->Allocate(true);
  }

  typename HessianImageType::Pointer hessianImage =
      dynamic_cast<HessianImageType*>(this->itk::ProcessObject::GetOutput(2));

  hessianImage->SetBufferedRegion(hessianImage->GetRequestedRegion());
  hessianImage->Allocate();

  AllocateUpdateBuffer();
  typename InputImageType::ConstPointer input = this->GetInput();

  this->m_HessianFilter->SetInput(input);
  this->m_HessianFilter->SetNormalizeAcrossScale(false);

  // Create a process accumulator for tracking the progress of this
  // minipipeline
  itk::ProgressAccumulator::Pointer progress = itk::ProgressAccumulator::New();
  progress->SetMiniPipelineFilter(this);

  if (m_NumberOfSigmaSteps > 0)
  {
    const double filterStep = 0.5 / m_NumberOfSigmaSteps;
    progress->RegisterInternalFilter(this->m_HessianFilter, filterStep);
    progress->RegisterInternalFilter(this->m_HessianToMeasureFilter,
                                     filterStep);
  }


  int scalemax= m_NumberOfSigmaSteps-1 ;
  for (int scaleLevel = scalemax; scaleLevel >= 0;
       --scaleLevel)
  {
    const double sigma = this->ComputeSigmaValue(scaleLevel);

    std::cout
        << "(In MultiScaleHessian) Computing measure for scale with sigma = "
        << sigma << std::endl;

    m_HessianFilter->SetSigma(sigma);
    m_HessianToMeasureFilter->SetInput(m_HessianFilter->GetOutput());

    /*
    m_HessianToMeasureFilter->GetFlippedHessian()->SetSpacing(this->GetOutput()->GetSpacing());
    m_HessianToMeasureFilter->GetFlippedHessian()->SetOrigin(this->GetOutput()->GetOrigin());
    m_HessianToMeasureFilter->GetFlippedHessian()->SetLargestPossibleRegion(this->GetOutput()->GetLargestPossibleRegion());
    m_HessianToMeasureFilter->GetFlippedHessian()->SetRequestedRegion(this->GetOutput()->GetRequestedRegion());
    m_HessianToMeasureFilter->GetFlippedHessian()->SetBufferedRegion(this->GetOutput()->GetBufferedRegion());
    m_HessianToMeasureFilter->GetFlippedHessian()->Allocate();
    m_HessianToMeasureFilter->GetFlippedHessian()->FillBuffer( itk::NumericTraits< double >::Zero );
    */

    std::cout << "..doing first pass to obtain strongest Lambda per scale" << std::endl ; 
    m_HessianToMeasureFilter->FirstPassOn();
    m_HessianToMeasureFilter->Update(); //First pass to update strongest Lambda3
    
    std::cout << "..doing second pass to compute regularized vesselness" << std::endl ; 
    m_HessianToMeasureFilter->FirstPassOff();
    m_HessianToMeasureFilter->Update();

    typedef itk::Image<double, ImageDimension> doubleImageType;
   

    //WRITE OUTPUT TO FILE
    std::string sig = std::to_string(int(sigma*100000));
    std::string padded_sig = sig.insert(0,7-sig.length(), '0');
    
    typedef itk::Image<float, ImageDimension> floatImageType;
    typedef itk::ImageFileWriter<floatImageType> ImageWriterType;
    typedef itk::CastImageFilter< doubleImageType, floatImageType > CastFilterType;
    typename CastFilterType::Pointer castFilterFirst = CastFilterType::New();
    castFilterFirst->SetInput(m_HessianToMeasureFilter->GetOutput());
 
    typename ImageWriterType::Pointer writerfirst = ImageWriterType::New();
    writerfirst->SetFileName("Scale_NOWEINER_" + padded_sig + "_Vesselness.nii.gz");
    writerfirst->SetInput(castFilterFirst->GetOutput());
    writerfirst->Update();
    //////////////////////

    /*//////////////
    const unsigned int vectorlength = 6; 
    typedef itk::Vector<double, vectorlength> EigenVectorType;
    typedef itk::Image<EigenVectorType, ImageDimension> newHessianImageType;
    typedef itk::ImageFileWriter<newHessianImageType> ImageVectorWriterType;
    typename ImageVectorWriterType::Pointer writerVec = ImageVectorWriterType::New();
    writerVec->SetFileName("Hessian_" + padded_sig + "__D11_D22_D33_D12_D13_D23.nii.gz");
    writerVec->SetInput(m_HessianToMeasureFilter->GetFlippedHessian());
    writerVec->Update();
    //////////////*/

  
    //APPLY A CONVOLUTION FILTER TO REVERSE THE SMOOTHING EFFECT (maybe try weiner)
    typedef itk::ProjectedLandweberDeconvolutionImageFilter<doubleImageType>  InverseFilterImageType;
    //typedef itk::RichardsonLucyDeconvolutionImageFilter<doubleImageType>  InverseFilterImageType;
    
    typename  InverseFilterImageType::Pointer InverseFilterType =  InverseFilterImageType::New();
    
    typename doubleImageType::Pointer gaussKernel = doubleImageType::New();
    typename doubleImageType::IndexType start;
      start.Fill(0);
    typename doubleImageType::SizeType size;
      int sizeodd = 2 * ( (int)( 9*sigma / 2.0f ) ) + 1 ; //9 times.. might be 3 or 6
      sizeodd = vnl_math_max(7, sizeodd);
      std::cout << "..filter size is: " << std::to_string(sizeodd) << std::endl ; 
      size.Fill(sizeodd);
    typename doubleImageType::RegionType region;
      region.SetSize(size);
      region.SetIndex(start);
    typename doubleImageType::IndexType pixelIndex;
      pixelIndex[0] = (size[0]-1)/2;
      pixelIndex[1] = (size[1]-1)/2;
      pixelIndex[2] = (size[2]-1)/2;

    gaussKernel->SetRegions(region);
    //gaussKernel->SetOrigin(this->GetInput()->GetOrigin());
    //gaussKernel->SetSpacing(this->GetInput()->GetSpacing());
    gaussKernel->Allocate();
    gaussKernel->SetPixel(pixelIndex, 1.0);
    gaussKernel->Update();
  
    typedef itk::DiscreteGaussianImageFilter<doubleImageType, doubleImageType> gaussBlurType;
    typename gaussBlurType::Pointer blurfilter = gaussBlurType::New();
    blurfilter->SetInput( gaussKernel );
    blurfilter->SetVariance( 2.0*sigma );
    blurfilter->SetMaximumKernelWidth( sizeodd );
    blurfilter->Update();

    typedef itk::ThresholdImageFilter <doubleImageType> ThresholdImageFilterType;
    typename ThresholdImageFilterType::Pointer thresholdBelow  = ThresholdImageFilterType::New();
    if (sigma >= 99.35) //deconvolution relevant here
    {
      std::cout << "..deconvolve the scale with a R-L filter" << std::endl ; 
      InverseFilterType->SetInput(m_HessianToMeasureFilter->GetOutput());
      InverseFilterType->SetKernelImage(blurfilter->GetOutput());
      InverseFilterType->Update();

      thresholdBelow->SetInput(InverseFilterType->GetOutput());
    }
    else
    {
      thresholdBelow->SetInput(m_HessianToMeasureFilter->GetOutput());
    }
    thresholdBelow->ThresholdBelow(0.0001);
    thresholdBelow->SetOutsideValue(0);


//Ignore above, it gives crappy results for Clarity
    
    /*
    typedef itk::ThresholdImageFilter <doubleImageType> ThresholdImageFilterType;
    typename ThresholdImageFilterType::Pointer thresholdBelow  = ThresholdImageFilterType::New();
    thresholdBelow->SetInput(m_HessianToMeasureFilter->GetOutput());
    thresholdBelow->ThresholdBelow(0.0001);
    thresholdBelow->SetOutsideValue(0);*/


    /*typename ThresholdImageFilterType::Pointer thresholdUpper  = ThresholdImageFilterType::New();
    thresholdUpper->SetInput(thresholdBelow->GetOutput());
    thresholdUpper->ThresholdAbove(1.0);
    thresholdUpper->SetOutsideValue(1.0);*/

    typedef itk::LaplacianSharpeningImageFilter <doubleImageType, doubleImageType> LaplacianSharpeningFilterType;
    typename LaplacianSharpeningFilterType::Pointer sharpened  = LaplacianSharpeningFilterType::New();
    sharpened->SetInput(thresholdBelow->GetOutput());
    
    //WRITE OUTPUT TO FILE
    typename CastFilterType::Pointer castFilter = CastFilterType::New();
    castFilter->SetInput(sharpened->GetOutput());
 
    typename ImageWriterType::Pointer writer = ImageWriterType::New();
    writer->SetFileName("Scale_processed_" + padded_sig + "_Vesselness.nii.gz");
    writer->SetInput(castFilter->GetOutput());
    writer->Update();
    //////////////////////
    
    
    typedef itk::SqrtImageFilter<doubleImageType,doubleImageType> SqrtFilterType;
    typename SqrtFilterType::Pointer sqrter = SqrtFilterType::New();
    sqrter->SetInput(sharpened->GetOutput());
    
    //typedef itk::ThresholdImageFilter <doubleImageType> ThresholdImageFilterType;
    typename ThresholdImageFilterType::Pointer thresholdUpper  = ThresholdImageFilterType::New();
    thresholdUpper->SetInput(sqrter->GetOutput());
    thresholdUpper->ThresholdAbove(20.0);
    thresholdUpper->SetOutsideValue(20.0);
    
    typedef itk::RescaleIntensityImageFilter<doubleImageType> RescaleFilterType;
    typename RescaleFilterType::Pointer rescaler = RescaleFilterType::New();
    rescaler->SetOutputMinimum(   0 );
    rescaler->SetOutputMaximum( 1000 );
    rescaler->SetInput(thresholdUpper->GetOutput());
    
    
    //WRITE OUTPUT TO FILE
    castFilter->SetInput(rescaler->GetOutput());
    writer->SetFileName("Scale_rescaled_" + padded_sig + "_Vesselness.nii.gz");
    writer->SetInput(castFilter->GetOutput());
    writer->Update();
    //////////////////////
  
    this->UpdateMaximumResponse(sigma);
  }

  // Write out the best response to the output image.
  const OutputRegionType outputRegion = this->GetOutput()->GetBufferedRegion();
  itk::ImageRegionIterator<UpdateBufferType> itUpdate(m_UpdateBuffer,
                                                      outputRegion);
  itUpdate.GoToBegin();

  itk::ImageRegionIterator<TOutputImage> itOutput(this->GetOutput(),
                                                  outputRegion);
  itOutput.GoToBegin();

  while (!itOutput.IsAtEnd())
  {
    itOutput.Value() = static_cast<OutputPixelType>(itUpdate.Get());
    ++itOutput;
    ++itUpdate;
  }
  m_UpdateBuffer->ReleaseData();
}


// =============================================================================
// Keep the best sigma scale in memory (Hessian, scale and vesselness)
// =============================================================================
template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::UpdateMaximumResponse(double sigma)
{

  const OutputRegionType outputRegion = this->GetOutput()->GetBufferedRegion();

  itk::ImageRegionIterator<UpdateBufferType> itOutput(m_UpdateBuffer,
                                                      outputRegion);

  typename ScalesImageType::Pointer scalesImage =
      dynamic_cast<ScalesImageType*>(this->itk::ProcessObject::GetOutput(1));
  itk::ImageRegionIterator<ScalesImageType> itOutputScale;

  typename HessianImageType::Pointer hessianImage =
      dynamic_cast<HessianImageType*>(this->itk::ProcessObject::GetOutput(2));
  itk::ImageRegionIterator<HessianImageType> itHessian;

  itOutput.GoToBegin();
  if (m_GenerateScalesOutput)
  {
    itOutputScale =
        itk::ImageRegionIterator<ScalesImageType>(scalesImage, outputRegion);
    itOutputScale.GoToBegin();
  }
  itHessian =
      itk::ImageRegionIterator<HessianImageType>(hessianImage, outputRegion);
  itHessian.GoToBegin();

  typedef typename HessianToMeasureFilterType::OutputImageType
      HessianToMeasureOutputImageType;

  itk::ImageRegionIterator<HessianToMeasureOutputImageType> itHessianOutput(
      m_HessianToMeasureFilter->GetOutput(), outputRegion);
  itk::ImageRegionIterator<HessianImageType> itHessianImage(
      m_HessianFilter->GetOutput(), outputRegion);

  itHessianOutput.GoToBegin();
  itHessianImage.GoToBegin();

  while (!itOutput.IsAtEnd())
  {
    if (itOutput.Value() < itHessianOutput.Value())
    {
      itOutput.Value() = itHessianOutput.Value();

      if (m_GenerateScalesOutput)
      {
        itOutputScale.Value() = static_cast<ScalesPixelType>(sigma);
      }
      itHessian.Value() = itHessianImage.Value();
    }
    ++itOutput;
    ++itHessianOutput;
    if (m_GenerateScalesOutput)
    {
      ++itOutputScale;
    }
    ++itHessian;
    ++itHessianImage;
  }
}

// =============================================================================
// Generate the different sigma scale based on user parameters.
// =============================================================================
template <typename TInputImage, typename THessianImage, typename TOutputImage>
double
MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::ComputeSigmaValue(
    int scaleLevel)
{
  if (m_NumberOfSigmaSteps < 2)
  {
    return m_SigmaMinimum;
  }

  //HACK:
  m_SigmaStepMethod = Self::LogarithmicSigmaSteps;

  switch (m_SigmaStepMethod)
  {
  case Self::EquispacedSigmaSteps:
  {
    const double stepSize = vnl_math_max(
        1e-10, (m_SigmaMaximum - m_SigmaMinimum) / (m_NumberOfSigmaSteps - 1));
    return m_SigmaMinimum + stepSize * scaleLevel;
  }
  case Self::LogarithmicSigmaSteps:
  {
    const double stepSize = vnl_math_max(
        1e-10, (std::log(m_SigmaMaximum) - std::log(m_SigmaMinimum)) /
                   (m_NumberOfSigmaSteps - 1));
    return std::exp(std::log(m_SigmaMinimum) + stepSize * scaleLevel);
  }
  default:
    throw itk::ExceptionObject(__FILE__, __LINE__, "Invalid SigmaStepMethod.",
                               ITK_LOCATION);
    break;
  }
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::SetSigmaStepMethodToEquispaced()
{
  this->SetSigmaStepMethod(Self::EquispacedSigmaSteps);
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage,
                       TOutputImage>::SetSigmaStepMethodToLogarithmic()
{
  this->SetSigmaStepMethod(Self::LogarithmicSigmaSteps);
}

// Returns the image containing the best Hessian matrix at each voxel where the
// vesselness measure response was the highest.
template <typename TInputImage, typename THessianImage, typename TOutputImage>
const typename MultiScaleHessian<TInputImage, THessianImage,
                                 TOutputImage>::HessianImageType*
MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::GetHessianOutput()
    const
{
  return dynamic_cast<const HessianImageType*>(
      this->itk::ProcessObject::GetOutput(2));
}

// Returns the image containing the best sigma scale at each voxel where the
// vesselness measure response was the highest.
template <typename TInputImage, typename THessianImage, typename TOutputImage>
const typename MultiScaleHessian<TInputImage, THessianImage,
                                 TOutputImage>::ScalesImageType*
MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::GetScalesOutput()
    const
{
  return dynamic_cast<const ScalesImageType*>(
      this->itk::ProcessObject::GetOutput(1));
}

template <typename TInputImage, typename THessianImage, typename TOutputImage>
void MultiScaleHessian<TInputImage, THessianImage, TOutputImage>::PrintSelf(
    std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "SigmaMinimum:  " << m_SigmaMinimum << std::endl;
  os << indent << "SigmaMaximum:  " << m_SigmaMaximum << std::endl;
  os << indent << "NumberOfSigmaSteps:  " << m_NumberOfSigmaSteps << std::endl;
  os << indent << "SigmaStepMethod:  " << m_SigmaStepMethod << std::endl;
  os << indent << "HessianToMeasureFilter: " << m_HessianToMeasureFilter
     << std::endl;
  os << indent
     << "NonNegativeHessianBasedMeasure:  " << m_NonNegativeHessianBasedMeasure
     << std::endl;
  os << indent << "GenerateScalesOutput: " << m_GenerateScalesOutput
     << std::endl;
  os << indent << "GenerateHessianOutput: " << m_GenerateHessianOutput
     << std::endl;
}

#endif
