#include <vtkAnimationCue.h>
#include <vtkAnimationScene.h>
#include <vtkCommand.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include <iostream>

namespace {
class CueAnimator
{
public:
  CueAnimator()
  {
    this->SphereSource = nullptr;
    this->Mapper = nullptr;
    this->Actor = nullptr;
  }

  ~CueAnimator()
  {
    this->Cleanup();
  }

  void StartCue(vtkAnimationCue::AnimationCueInfo* vtkNotUsed(info),
                vtkRenderer* ren)
  {
    vtkNew<vtkNamedColors> colors;

    std::cout << "*** IN StartCue " << endl;
    this->SphereSource = vtkSphereSource::New();
    this->SphereSource->SetRadius(0.5);

    this->Mapper = vtkPolyDataMapper::New();
    this->Mapper->SetInputConnection(this->SphereSource->GetOutputPort());

    this->Actor = vtkActor::New();
    this->Actor->SetMapper(this->Mapper);
    this->Actor->GetProperty()->SetSpecular(0.6);
    this->Actor->GetProperty()->SetSpecularPower(30);
    this->Actor->GetProperty()->SetColor(
        colors->GetColor3d("Peacock").GetData());

    ren->AddActor(this->Actor);
    ren->ResetCamera();
    ren->Render();
  }

  void Tick(vtkAnimationCue::AnimationCueInfo const* info, vtkRenderer* ren)
  {
    double newradius = 0.1 +
        (static_cast<double>(info->AnimationTime - info->StartTime) /
         static_cast<double>(info->EndTime - info->StartTime)) *
            1;
    this->SphereSource->SetRadius(newradius);
    this->SphereSource->Update();
    ren->Render();
  }

  void EndCue(vtkAnimationCue::AnimationCueInfo* vtkNotUsed(info),
              vtkRenderer* ren)
  {
    // (void)ren;
    // don't remove the actor for the regression image.
    //      ren->RemoveActor(this->Actor);
    this->Cleanup();
  }

protected:
  vtkSphereSource* SphereSource;
  vtkPolyDataMapper* Mapper;
  vtkActor* Actor;

  void Cleanup()
  {
    if (this->SphereSource != nullptr)
    {
      this->SphereSource->Delete();
      this->SphereSource = nullptr;
    }

    if (this->Mapper != nullptr)
    {
      this->Mapper->Delete();
      this->Mapper = nullptr;
    }
    if (this->Actor != nullptr)
    {
      this->Actor->Delete();
      this->Actor = nullptr;
    }
  }
};

class vtkAnimationCueObserver : public vtkCommand
{
public:
  static vtkAnimationCueObserver* New()
  {
    return new vtkAnimationCueObserver;
  }

  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long event,
                       void* calldata)
  {
    if (this->Animator != nullptr && this->Renderer != nullptr)
    {
      vtkAnimationCue::AnimationCueInfo* info =
          static_cast<vtkAnimationCue::AnimationCueInfo*>(calldata);
      switch (event)
      {
      case vtkCommand::StartAnimationCueEvent:
        this->Animator->StartCue(info, this->Renderer);
        break;
      case vtkCommand::EndAnimationCueEvent:
        this->Animator->EndCue(info, this->Renderer);
        break;
      case vtkCommand::AnimationCueTickEvent:
        this->Animator->Tick(info, this->Renderer);
        break;
      }
    }
    if (this->RenWin != nullptr)
    {
      this->RenWin->Render();
    }
  }

  vtkRenderer* Renderer;
  vtkRenderWindow* RenWin;
  CueAnimator* Animator;

protected:
  vtkAnimationCueObserver()
  {
    this->Renderer = nullptr;
    this->Animator = nullptr;
    this->RenWin = nullptr;
  }
};
} // namespace

int main(int, char*[])
{
  vtkNew<vtkNamedColors> colors;

  // Create the graphics structure. The renderer renders into the
  // render window.
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetWindowName("AnimationScene");

  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(ren1);
  ren1->SetBackground(colors->GetColor3d("MistyRose").GetData());
  renWin->Render();

  // Create an Animation Scene
  vtkNew<vtkAnimationScene> scene;

  scene->SetModeToRealTime();
  // scene->SetModeToSequence();

  scene->SetLoop(0);
  scene->SetFrameRate(5);
  scene->SetStartTime(3);
  scene->SetEndTime(20);

  // Create an Animation Cue.
  vtkNew<vtkAnimationCue> cue1;
  cue1->SetStartTime(5);
  cue1->SetEndTime(23);
  scene->AddCue(cue1);

  // Create cue animator;
  CueAnimator animator;

  // Create Cue observer.
  vtkNew<vtkAnimationCueObserver> observer;
  observer->Renderer = ren1;
  observer->Animator = &animator;
  observer->RenWin = renWin;

  cue1->AddObserver(vtkCommand::StartAnimationCueEvent, observer);
  cue1->AddObserver(vtkCommand::EndAnimationCueEvent, observer);
  cue1->AddObserver(vtkCommand::AnimationCueTickEvent, observer);

  scene->Play();
  scene->Stop();

  iren->Start();

  return EXIT_SUCCESS;
}
