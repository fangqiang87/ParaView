/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVXRInterfaceWidgets
 * @brief   Support for widgets in XR
 *
 */

#ifndef vtkPVXRInterfaceWidgets_h
#define vtkPVXRInterfaceWidgets_h

#include "vtkNew.h" // for ivars
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for ivars

#include <future> // for ivar
#include <map>    // for ivar
#include <vector> // for ivar

class vtkBoxWidget2;
class vtkDataSetAttributes;
class vtkEventData;
class vtkEventDataDevice3D;
class vtkImageData;
class vtkImplicitPlaneWidget2;
class vtkOpenGLRenderWindow;
class vtkPVXRInterfaceHelper;
class vtkPVXRInterfaceHelperLocation;
class vtkStringArray;
class vtkTexture;
class vtkTransform;

class vtkPVXRInterfaceWidgets : public vtkObject
{
public:
  static vtkPVXRInterfaceWidgets* New();
  vtkTypeMacro(vtkPVXRInterfaceWidgets, vtkObject);

  void SetShowNavigationPanel(bool val, vtkOpenGLRenderWindow*);
  bool GetNavigationPanelVisibility();
  void UpdateNavigationText(vtkEventDataDevice3D* edd, vtkOpenGLRenderWindow*);

  void TakeMeasurement(vtkOpenGLRenderWindow*);
  void RemoveMeasurement();

  // show the billboard with the provided text
  void ShowBillboard(std::string const& text, bool updatePosition, std::string const& tfile);
  void HideBillboard();
  void MoveToNextImage();
  void MoveToNextCell();
  void UpdateBillboard(bool updatePosition);

  ///@{
  /**
   * Add/remove crop planes and thick crops
   */
  void AddACropPlane(double* origin, double* normal);
  void collabAddACropPlane(double* origin, double* normal);
  void collabRemoveAllCropPlanes();
  void collabUpdateCropPlane(int count, double* origin, double* normal);
  void AddAThickCrop(vtkTransform* t);
  void collabAddAThickCrop(vtkTransform* t);
  void collabRemoveAllThickCrops();
  void collabUpdateThickCrop(int count, double* matrix);
  void MoveThickCrops(bool forward);
  void SetCropSnapping(int val);
  bool GetCropSnapping() { return this->CropSnapping; }
  ///@}

  size_t GetNumberOfCropPlanes() { return this->CropPlanes.size(); }

  size_t GetNumberOfThickCrops() { return this->ThickCrops.size(); }

  ///@{
  /**
   * set the initial thickness in world coordinates for
   * thick crop planes. 0 indicates automatic
   * setting. It defaults to 0
   */
  vtkSetMacro(DefaultCropThickness, double);
  vtkGetMacro(DefaultCropThickness, double);
  ///@}

  ///@{
  /**
   * allow the user to edit a scalar field
   * in VR
   */
  vtkSetMacro(EditableField, std::string);
  vtkGetMacro(EditableField, std::string);
  ///@}

  void SetEditableFieldValue(std::string name);

  void HandlePickEvent(vtkObject* caller, void* calldata);
  void SetLastEventData(vtkEventData* edd);

  void SetHelper(vtkPVXRInterfaceHelper*);

  /**
   * write any widget state that needs to be in the location state
   */
  void SaveLocationState(vtkPVXRInterfaceHelperLocation& sd);

  void ReleaseGraphicsResources();

  bool LoginToImago(std::string const& uid, std::string const& pw);
  void SetImagoWorkspace(std::string val);
  void SetImagoDataset(std::string val);
  void SetImagoImageryType(std::string val);
  void SetImagoImageType(std::string val);
  void GetImagoWorkspaces(std::vector<std::string>& vals);
  void GetImagoDatasets(std::vector<std::string>& vals);
  void GetImagoImageryTypes(std::vector<std::string>& vals);
  void GetImagoImageTypes(std::vector<std::string>& vals);

  void UpdateWidgetsFromParaView();

  /**
   * perform any cleanup required when quitting VR
   */
  void Quit();

protected:
  vtkPVXRInterfaceWidgets();
  ~vtkPVXRInterfaceWidgets();

  bool HasCellImage(vtkStringArray* sa, vtkIdType currCell);
  bool FindCellImage(vtkDataSetAttributes* celld, vtkIdType currCell, std::string& image);
  bool IsCellImageDifferent(std::string const& oldimg, std::string const& newimg);
  bool EventCallback(vtkObject* object, unsigned long event, void* calldata);
  void UpdateTexture();

private:
  vtkPVXRInterfaceWidgets(const vtkPVXRInterfaceWidgets&) = delete;
  void operator=(const vtkPVXRInterfaceWidgets&) = delete;

  std::vector<vtkImplicitPlaneWidget2*> CropPlanes;
  std::vector<vtkBoxWidget2*> ThickCrops;
  bool CropSnapping = false;
  double DefaultCropThickness = 0;
  vtkPVXRInterfaceHelper* Helper = nullptr;

  bool WaitingForImage = false;
  std::future<vtkImageData*> ImageFuture;
  unsigned long RenderObserver = 0;
  std::string EditableField;
  vtkSmartPointer<vtkEventData> LastEventData;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
