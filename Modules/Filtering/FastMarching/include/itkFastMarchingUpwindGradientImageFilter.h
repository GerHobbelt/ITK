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
#ifndef itkFastMarchingUpwindGradientImageFilter_h
#define itkFastMarchingUpwindGradientImageFilter_h

#include "itkFastMarchingImageFilter.h"
#include "itkImage.h"

namespace itk
{

/** \class FastMarchingUpwindGradientImageFilterEnums
 *
 * \brief enums for itk::FastMarchingUpwindGradientImageFilter
 *
 * \ingroup ITKFastMarching
 */
class FastMarchingUpwindGradientImageFilterEnums
{
public:
  /** \class TargetCondition
   * \ingroup ITKFastMarching
   * Specify how many targets are necessary to determine when the front must stop.
   */
  enum class TargetCondition : int
  {
    NoTargets,
    OneTarget,
    SomeTargets,
    AllTargets
  };
};
/** Define how to print enumeration values. */
extern ITKFastMarching_EXPORT std::ostream &
                              operator<<(std::ostream & out, const FastMarchingUpwindGradientImageFilterEnums::TargetCondition value);


/**
 * \class FastMarchingUpwindGradientImageFilter
 *
 * \brief Generates the upwind gradient field of fast marching arrival times.
 *
 * This filter adds some extra functionality to its base class. While the
 * solution T(x) of the Eikonal equation is being generated by the base class
 * with the fast marching method, the filter generates the upwind gradient
 * vectors of T(x), storing them in an image.
 *
 * Since the Eikonal equation generates the arrival times of a wave traveling
 * at a given speed, the generated gradient vectors can be interpreted as the
 * slowness (1/velocity) vectors of the front (the quantity inside the modulus
 * operator in the Eikonal equation).
 *
 * Gradient vectors are computed using upwind finite differences, that is,
 * information only propagates from points where the wavefront has already
 * passed. This is consistent with how the fast marching method works.
 *
 * One more extra feature is the possibility to define a set of Target points
 * where the propagation stops. This can be used to avoid computing the Eikonal
 * solution for the whole domain.  The front can be stopped either when one
 * Target point is reached or all Target points are reached. The propagation
 * can stop after a time TargetOffset has passed since the stop condition is
 * met. This way the solution is computed a bit downstream the Target points,
 * so that the level sets of T(x) corresponding to the Target are smooth.
 *
 * For an alternative implementation, see itk::FastMarchingUpwindGradientImageFilterBase.
 *
 * \author Luca Antiga Ph.D.  Biomedical Technologies Laboratory,
 *                            Bioengineering Department, Mario Negri Institute, Italy.
 *
 * \ingroup ITKFastMarching
 */
template <typename TLevelSet, typename TSpeedImage = Image<float, TLevelSet::ImageDimension>>
class ITK_TEMPLATE_EXPORT FastMarchingUpwindGradientImageFilter : public FastMarchingImageFilter<TLevelSet, TSpeedImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(FastMarchingUpwindGradientImageFilter);

  /** Standard class typedefs. */
  using Self = FastMarchingUpwindGradientImageFilter;
  using Superclass = FastMarchingImageFilter<TLevelSet, TSpeedImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(FastMarchingUpwindGradientImageFilter);

  /** Inherited type alias. */
  using typename Superclass::LevelSetType;
  using typename Superclass::SpeedImageType;
  using typename Superclass::LevelSetImageType;
  using typename Superclass::LevelSetPointer;
  using typename Superclass::SpeedImageConstPointer;
  using typename Superclass::LabelImageType;
  using typename Superclass::PixelType;
  using typename Superclass::AxisNodeType;
  using typename Superclass::NodeType;
  using typename Superclass::NodeContainer;
  using typename Superclass::NodeContainerPointer;

  using typename Superclass::IndexType;
  using typename Superclass::OutputSpacingType;
  using typename Superclass::LevelSetIndexType;

  using PointType = typename Superclass::OutputPointType;

  /** The dimension of the level set. */
  static constexpr unsigned int SetDimension = Superclass::SetDimension;

  using TargetConditionEnum = FastMarchingUpwindGradientImageFilterEnums::TargetCondition;
  /** Backwards compatibility for enum values */
#if !defined(ITK_LEGACY_REMOVE)
  // We need to expose the enum values at the class level
  // for backwards compatibility
  static constexpr TargetConditionEnum NoTargets = TargetConditionEnum::NoTargets;
  static constexpr TargetConditionEnum OneTarget = TargetConditionEnum::OneTarget;
  static constexpr TargetConditionEnum SomeTargets = TargetConditionEnum::SomeTargets;
  static constexpr TargetConditionEnum AllTargets = TargetConditionEnum::AllTargets;
#endif

  /** Set the container of Target Points.
   * If a target point is reached, the propagation stops.
   * Trial points are represented as a VectorContainer of LevelSetNodes. */
  void
  SetTargetPoints(NodeContainer * points)
  {
    m_TargetPoints = points;
    this->Modified();
  }

  /** Get the container of Target Points. */
  NodeContainerPointer
  GetTargetPoints()
  {
    return m_TargetPoints;
  }

  /** Get the container of Reached Target Points. */
  NodeContainerPointer
  GetReachedTargetPoints()
  {
    return m_ReachedTargetPoints;
  }

  /** GradientPixel type alias support */
  using GradientPixelType = CovariantVector<PixelType, Self::SetDimension>;

  /** GradientImage type alias support */
  using GradientImageType = Image<GradientPixelType, Self::SetDimension>;

  /** GradientImagePointer type alias support */
  using GradientImagePointer = typename GradientImageType::Pointer;

  /** Get the gradient image. */
  GradientImagePointer
  GetGradientImage() const
  {
    return m_GradientImage;
  }

  /** Set the GenerateGradientImage flag. Instrument the algorithm to generate
   * the gradient of the Eikonal equation solution while fast marching. */
  itkSetMacro(GenerateGradientImage, bool);

  /** Get the GenerateGradientImage flag. */
  itkGetConstReferenceMacro(GenerateGradientImage, bool);
  itkBooleanMacro(GenerateGradientImage);

  /** Set how long (in terms of arrival times) after targets are reached the
   * front must stop.  This is useful to ensure that the level set of target
   * arrival time is smooth. */
  itkSetMacro(TargetOffset, double);
  /** Get the TargetOffset ivar. */
  itkGetConstReferenceMacro(TargetOffset, double);

  /** Choose whether the front must stop when the first target has been reached
   * or all targets have been reached.
   */
  itkSetMacro(TargetReachedMode, TargetConditionEnum);
  itkGetConstReferenceMacro(TargetReachedMode, TargetConditionEnum);
  void
  SetTargetReachedModeToNoTargets()
  {
    this->SetTargetReachedMode(TargetConditionEnum::NoTargets);
    m_NumberOfTargets = 0;
  }
  void
  SetTargetReachedModeToOneTarget()
  {
    this->SetTargetReachedMode(TargetConditionEnum::OneTarget);
    m_NumberOfTargets = 1;
  }
  void
  SetTargetReachedModeToSomeTargets(SizeValueType numberOfTargets)
  {
    this->SetTargetReachedMode(TargetConditionEnum::SomeTargets);
    m_NumberOfTargets = numberOfTargets;
  }

  void
  SetTargetReachedModeToAllTargets()
  {
    this->SetTargetReachedMode(TargetConditionEnum::AllTargets);
    // m_NumberOfTargets is not used for this case
  }

  /** Get the number of targets. */
  itkGetConstReferenceMacro(NumberOfTargets, SizeValueType);

  /** Get the arrival time corresponding to the last reached target.
   *  If TargetReachedMode is set to TargetConditionEnum::NoTargets, TargetValue contains
   *  the last (aka largest) Eikonal solution value generated.
   */
  itkGetConstReferenceMacro(TargetValue, double);

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro(LevelSetDoubleDivisionOperatorsCheck,
                  (Concept::DivisionOperators<typename TLevelSet::PixelType, double>));
  itkConceptMacro(LevelSetDoubleDivisionAndAssignOperatorsCheck,
                  (Concept::DivisionAndAssignOperators<typename TLevelSet::PixelType, double>));
  // End concept checking
#endif

protected:
  FastMarchingUpwindGradientImageFilter();
  ~FastMarchingUpwindGradientImageFilter() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  virtual void
  VerifyPreconditions() const override;

  void
  Initialize(LevelSetImageType *) override;

  void
  GenerateData() override;

  void
  UpdateNeighbors(const IndexType & index, const SpeedImageType *, LevelSetImageType *) override;

  virtual void
  ComputeGradient(const IndexType &         index,
                  const LevelSetImageType * output,
                  const LabelImageType *    labelImage,
                  GradientImageType *       gradientImage);

  /** Check that target points are set.
   *  Returns true if at least a target point exists; returns false otherwise.
   */
  bool
  IsTargetPointsExistenceConditionSatisfied() const
  {
    if (!m_TargetPoints || m_TargetPoints->Size() == 0)
    {
      return false;
    }
    else
    {
      return true;
    }
  }

  /** Check that the conditions to set the target reached mode are satisfied.
   *
   * The sufficient target point count is 1 for TargetConditionEnum::OneTarget and TargetConditionEnum::AllTargets
   * modes; and it is given by a particular value for the TargetConditionEnum::SomeTargets mode.
   *
   * Raises an exception if the conditions are not satisfied.
   */
  void
  VerifyTargetReachedModeConditions(unsigned int targetModeMinPoints = 1) const
  {
    bool targetPointsExist = this->IsTargetPointsExistenceConditionSatisfied();

    if (!targetPointsExist)
    {
      itkExceptionMacro("No target point set. Cannot set the target reached mode.");
    }
    else
    {
      SizeValueType availableNumberOfTargets = m_TargetPoints->Size();
      if (targetModeMinPoints > availableNumberOfTargets)
      {
        itkExceptionMacro("Not enough target points: Available: " << availableNumberOfTargets
                                                                  << "; Requested: " << targetModeMinPoints);
      }
    }
  }


private:
  NodeContainerPointer m_TargetPoints{};
  NodeContainerPointer m_ReachedTargetPoints{};

  GradientImagePointer m_GradientImage{};

  bool m_GenerateGradientImage{};

  double m_TargetOffset{};

  TargetConditionEnum m_TargetReachedMode{};

  double m_TargetValue{};

  SizeValueType m_NumberOfTargets{};
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkFastMarchingUpwindGradientImageFilter.hxx"
#endif

#endif
