/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkExpectationBasedPointSetToPointSetMetricv4.h"
#include "itkGradientDescentOptimizerv4.h"
#include "itkRegistrationParameterScalesFromShift.h"
#include "itkTranslationTransform.h"

#include <fstream>

int itkExpectationBasedPointSetMetricRegistrationTest( int argc, char *argv[] )
{
  const unsigned int Dimension = 2;

  unsigned int numberOfIterations = 10000;
  if( argc > 1 )
    {
    numberOfIterations = atoi( argv[1] );
    }

  typedef itk::PointSet<unsigned char, Dimension> PointSetType;

  typedef PointSetType::PointType PointType;

  PointSetType::Pointer fixedPoints = PointSetType::New();
  fixedPoints->Initialize();

  PointSetType::Pointer movingPoints = PointSetType::New();
  movingPoints->Initialize();

/*
  // two ellipses, one rotated slightly
  unsigned long count = 0;
  for( float theta = 0; theta < 2.0 * vnl_math::pi; theta += 0.1 )
    {
    PointType fixedPoint;
    fixedPoint[0] = 5.0 * vcl_cos( theta );
    fixedPoint[1] = 1.0 * vcl_sin( theta );
    fixedPoints->SetPoint( count, fixedPoint );

    PointType movingPoint;
    movingPoint[0] = 5.0 * vcl_cos( theta + 0.1 * vnl_math::pi );
    movingPoint[1] = 1.0 * vcl_sin( theta + 0.1 * vnl_math::pi );
    movingPoints->SetPoint( count, movingPoint );

    count++;
    }*/


  // two circles with a small offset
  PointType offset;
  for( unsigned int d=0; d < Dimension; d++ )
    {
    offset[d] = 2.0;
    }
  unsigned long count = 0;
  for( float theta = 0; theta < 2.0 * vnl_math::pi; theta += 0.1 )
    {
    PointType fixedPoint;
    float radius = 100.0;
    fixedPoint[0] = radius * vcl_cos( theta );
    fixedPoint[1] = radius * vcl_sin( theta );
    if( Dimension > 2 )
      {
      fixedPoint[2] = radius * vcl_sin( theta );
      }
    fixedPoints->SetPoint( count, fixedPoint );

    PointType movingPoint;
    movingPoint[0] = fixedPoint[0] + offset[0];
    movingPoint[1] = fixedPoint[1] + offset[1];
    if( Dimension > 2 )
      {
      movingPoint[2] = fixedPoint[2] + offset[2];
      }
    movingPoints->SetPoint( count, movingPoint );

    count++;
    }

  // Create a few points and apply a small rotation to make the moving point set
/*  float size = 100.0;
  float theta = vnl_math::pi / 180.0 * 10.0;
  unsigned int numberOfPoints = 4;
  PointType fixedPoint;
  fixedPoint[0] = 0;
  fixedPoint[1] = 0;
  fixedPoints->SetPoint( 0, fixedPoint );
  fixedPoint[0] = size;
  fixedPoint[1] = 0;
  fixedPoints->SetPoint( 1, fixedPoint );
  fixedPoint[0] = size;
  fixedPoint[1] = size;
  fixedPoints->SetPoint( 2, fixedPoint );
  fixedPoint[0] = 0;
  fixedPoint[1] = size;
  fixedPoints->SetPoint( 3, fixedPoint );
  PointType movingPoint;
  for( unsigned int n=0; n < numberOfPoints; n ++ )
    {
    fixedPoint = fixedPoints->GetPoint( n );
    movingPoint[0] = fixedPoint[0] * vcl_cos( theta ) - fixedPoint[1] * vcl_sin( theta );
    movingPoint[1] = fixedPoint[0] * vcl_sin( theta ) + fixedPoint[1] * vcl_cos( theta );
    movingPoints->SetPoint( n, movingPoint );
    std::cout << fixedPoint << " -> " << movingPoint << std::endl;
    }
*/
  typedef itk::TranslationTransform<double, Dimension> TranslationTransformType;
  TranslationTransformType::Pointer translationTransform = TranslationTransformType::New();
  translationTransform->SetIdentity();

  // Instantiate the metric
  typedef itk::ExpectationBasedPointSetToPointSetMetricv4<PointSetType> PointSetMetricType;
  PointSetMetricType::Pointer metric = PointSetMetricType::New();
  metric->SetFixedPointSet( fixedPoints );
  metric->SetMovingPointSet( movingPoints );
  metric->SetPointSetSigma( 2.0 );
  metric->SetEvaluationKNeighborhood( 10 );
  metric->SetMovingTransform( translationTransform );
  metric->Initialize();

  // optimizer
  typedef itk::GradientDescentOptimizerv4  OptimizerType;
  OptimizerType::Pointer  optimizer = OptimizerType::New();
  optimizer->SetMetric( metric );
  optimizer->SetNumberOfIterations( numberOfIterations );
  //optimizer->SetScalesEstimator( shiftScaleEstimator );

  // create scales to normalize between translation and rotation components
  OptimizerType::ScalesType scales( metric->GetNumberOfParameters() );
  scales.Fill(0.1);

  optimizer->SetScales( scales );
  optimizer->SetLearningRate( 0.1 );
  optimizer->SetMinimumConvergenceValue( 0.0 );
  optimizer->SetConvergenceWindowSize( 10 );
  optimizer->StartOptimization();

  std::cout << "numberOfIterations: " << numberOfIterations << std::endl;
  std::cout << "Moving-source final value: " << optimizer->GetValue() << std::endl;
  std::cout << "Moving-source final position: " << optimizer->GetCurrentPosition() << std::endl;

  // applying the resultant transform to moving points and verify result
  std::cout << "Fixed\tMoving\tMoving Transformed\tDiff" << std::endl;
  bool passed = true;
  PointType::ValueType tolerance = 1e-4;
  TranslationTransformType::InverseTransformBasePointer movingInverse = metric->GetMovingTransform()->GetInverseTransform();
  TranslationTransformType::InverseTransformBasePointer fixedInverse = metric->GetFixedTransform()->GetInverseTransform();
  for( unsigned int n=0; n < metric->GetNumberOfComponents(); n++ )
    {
    // compare the points in virtual domain
    PointType transformedMovingPoint = movingInverse->TransformPoint( movingPoints->GetPoint( n ) );
    PointType transformedFixedPoint = fixedInverse->TransformPoint( fixedPoints->GetPoint( n ) );
    PointType difference;
    difference[0] = transformedMovingPoint[0] - transformedFixedPoint[0];
    difference[1] = transformedMovingPoint[1] - transformedFixedPoint[1];
    std::cout << fixedPoints->GetPoint( n ) << "\t" << movingPoints->GetPoint( n )
          << "\t" << transformedMovingPoint << "\t" << transformedFixedPoint << "\t" << difference << std::endl;
    if( fabs( difference[0] ) > tolerance || fabs( difference[1] ) > tolerance )
      {
      passed = false;
      }
    }
  if( ! passed )
    {
    std::cerr << "Results do not match truth within tolerance." << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}