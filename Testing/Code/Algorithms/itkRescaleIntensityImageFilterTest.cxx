/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkRescaleIntensityImageFilterTest.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include <iostream>

#include "itkImage.h"
#include "itkImageRegionIterator.h"

#include "itkRescaleIntensityImageFilter.h"
#include "itkRandomImageSource.h"

int itkRescaleIntensityImageFilterTest(int, char**)
{
  std::cout << "itkRescaleIntensityImageFilterTest Start" << std::endl;

  typedef itk::Image<char,3> TestInputImage;
  typedef itk::Image<float,3> TestOutputImage;

  TestInputImage::Pointer    inputImage  = TestInputImage::New();
  TestInputImage::RegionType region;
  TestInputImage::SizeType   size; size.Fill(64);
  TestInputImage::IndexType  index; index.Fill(0);

  region.SetIndex (index);
  region.SetSize (size);

  // first try a constant image
  double fillValue = -100.0;
  inputImage->SetRegions( region );
  inputImage->Allocate();
  inputImage->FillBuffer( static_cast< TestInputImage::PixelType >( fillValue ) );

  typedef itk::RescaleIntensityImageFilter<TestInputImage,TestOutputImage> FilterType;
  FilterType::Pointer filter = FilterType::New();

  filter->DebugOn();

  // Filter the image
  filter->SetInput (inputImage);
  filter->UpdateLargestPossibleRegion();

  // Now generate a real image

  typedef itk::RandomImageSource<TestInputImage> SourceType;
  SourceType::Pointer source = SourceType::New();
  unsigned long randomSize[3] = {17, 8, 20};


  // Set up source
  source->SetSize(randomSize);
  double minValue = -128.0;
  double maxValue = 127.0;

  source->SetMin( static_cast< TestInputImage::PixelType >( minValue ) );
  source->SetMax( static_cast< TestInputImage::PixelType >( maxValue ) );
  std::cout << source;
  
  filter->SetInput(source->GetOutput());

  const double desiredMinimum = -1.0;
  const double desiredMaximum =  1.0;

  filter->SetOutputMinimum( desiredMinimum );
  filter->SetOutputMaximum( desiredMaximum );
  try
    {
    filter->UpdateLargestPossibleRegion();
    }
  catch (itk::ExceptionObject& e)
    {
    std::cerr << "Exception detected: "  << e;
    return -1;
    }
  
  typedef itk::MinimumMaximumImageCalculator< TestOutputImage > CalculatorType;

  CalculatorType::Pointer calculator  =  CalculatorType::New();

  calculator->SetImage( filter->GetOutput() );

  const double tolerance = 1e-7;

  const double obtainedMinimum = calculator->GetMinimum();
  const double obtainedMaximum = calculator->GetMaximum();

  if( vnl_math_abs( obtainedMinimum - desiredMinimum ) > tolerance )
    {
    std::cerr << "Error in minimum" << std::endl;
    std::cerr << "Expected minimum = " << desiredMinimum  << std::endl;
    std::cerr << "Obtained minimum = " << obtainedMinimum << std::endl;
    return EXIT_FAILURE;
    }

  if( vnl_math_abs( obtainedMaximum - desiredMaximum ) > tolerance )
    {
    std::cerr << "Error in minimum" << std::endl;
    std::cerr << "Expected minimum = " << desiredMaximum  << std::endl;
    std::cerr << "Obtained minimum = " << obtainedMaximum << std::endl;
    return EXIT_FAILURE;
    }

  
  return EXIT_SUCCESS;

}
